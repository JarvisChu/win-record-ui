#include "stdafx.h"
#include <stdio.h>
#include "record.h"
#include <chrono>
#include <thread>
#include <Windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <time.h>
#include <sstream>
#include <excpt.h>

using namespace std;

#pragma comment(lib, "winmm.lib")

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres, msg) { \
	if (FAILED(hres)) { \
		bHasError = true; \
		std::ostringstream stream; \
		stream << ", hr=0x" << hex << hres; \
		strErrMsg = msg + stream.str(); \
		LERROR(L"%s", UTF82Wide(strErrMsg).c_str());\
		if(bCoInitializeExSucc && !bCrashed){ \
			if(pwfx) CoTaskMemFree(pwfx); \
			SAFE_RELEASE(pEnumerator) \
			SAFE_RELEASE(pDevice) \
			SAFE_RELEASE(pAudioClient) \
			SAFE_RELEASE(pCaptureClient) \
			CoUninitialize(); \
		} \
		goto Exit; \
	} \
}

#define SAFE_RELEASE(punk) { if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; } }
                
const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);


std::string Record::GetCurrentTime(){
	time_t t = time(NULL);
	char ch[64] = { 0 };
	struct tm st;
	localtime_s(&st, &t);
	strftime(ch, sizeof(ch) - 1, "%Y_%m_%d_%H_%M_%S", &st);
	return std::string(ch);
}

Record::Record() {
	LINFO(L"[%ld]Record::Record\n", GetCurrentThreadId());
}

Record::~Record() {
	LINFO(L"[%ld]Record::~Record\n", GetCurrentThreadId());
	Stop();
}

void Record::Start(std::string audio_format, int sample_rate/* = 8000*/, int sample_bits/* = 16*/, int channel/* = 1*/) {
	LOGGER;
	if (m_running) {
		LINFO(L"already running");
		return;
	}

	// check audio_format
	AudioFormat af = AF_PCM;
	if (audio_format == "pcm") {
		af = AF_PCM;
	}else if (audio_format == "silk") {
		af = AF_SILK;
	}else {
		LERROR(L"unsupported audio format, only support pcm/silk currently");
		AfxMessageBox(L"unsupported audio format, only support pcm/silk currently");
		return;
	}

	//check sample_rate
	if(sample_rate != 8000 && sample_rate != 16000 && sample_rate != 24000 && 
		sample_rate != 32000 && sample_rate != 44100 && sample_rate != 48000){
		AfxMessageBox(L"invalid sample rate, only support 8000/16000/24000/32000/44100/48000");
		return;
	}

	// check sample_bit
	if(sample_bits != 8 && sample_bits != 16 && sample_bits != 24 && sample_bits != 32){
		AfxMessageBox(L"invalid sample bit, only support 8/16/24/32");
		return;
	}

	// check channel
	if(channel != 1 && channel != 2){
		AfxMessageBox(L"invalid channel number, only support 1/2");
		return;
	}

	LINFO(L"Record::Record audio_format:%s, sample_rate:%d, sample_bit:%d, channel:%d \n", UTF82Wide(audio_format).c_str(), sample_rate, sample_bits, channel);

	m_running = true;

	// file save path prefix, e.g. C:/path/to/dir/audio_filename
	std::string prefix = GetCurrentTime();
	m_ap_i.SetTgtAudioParam(af, sample_rate, sample_bits, channel, prefix+"_in");
	std::thread tmpI(&Record::Run, this, Wave_In);
	m_record_thread_i.swap(tmpI);

	m_ap_o.SetTgtAudioParam(af, sample_rate, sample_bits, channel, prefix + "_out");
	std::thread tmpO(&Record::Run, this, Wave_Out);
	m_record_thread_o.swap(tmpO);

	std::thread tmpS(&Record::RunSimulate, this);
	m_simulate_using_thread.swap(tmpS);
}

void Record::Stop() {
	LOGGER;
	if (m_running) {
		m_running = false;
		if (m_record_thread_i.joinable()) {
			m_record_thread_i.join();
		}
		if (m_record_thread_o.joinable()) {
			m_record_thread_o.join();
		}
		if (m_simulate_using_thread.joinable()) {
			m_simulate_using_thread.join();
		}

		m_ap_o.Stop();
		m_ap_i.Stop();
	}
}

void Record::Run(WaveSource ws){
	LOGGER;
	LINFO(L"[%ld]Record::Run, ws:%d\n", GetCurrentThreadId(), ws);

	IMMDeviceEnumerator *pEnumerator = NULL;
	IMMDevice *pDevice = NULL;
	IAudioClient *pAudioClient = NULL;
	WAVEFORMATEX *pwfx = NULL;
	IAudioCaptureClient *pCaptureClient = NULL;
	bool bHasError = false;
	std::string strErrMsg;
	bool bCoInitializeExSucc = false;
	bool bCrashed = false;
	int nRetryCnt = 0;

Recording:

	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	EXIT_ON_ERROR(hr, "CoInitializeEx failed");
	bCoInitializeExSucc = true;

	__try{
		hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL,CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&pEnumerator);
		EXIT_ON_ERROR(hr, "CoCreateInstance failed");

		EDataFlow device = eCapture;
		if (ws == Wave_Out) {
			device = eRender;
		}
		
		hr = pEnumerator->GetDefaultAudioEndpoint(device, eConsole, &pDevice);
		EXIT_ON_ERROR(hr, "GetDefaultAudioEndpoint failed");

		hr = pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&pAudioClient);
		EXIT_ON_ERROR(hr, "Activate failed");
		
		hr = pAudioClient->GetMixFormat(&pwfx);
		EXIT_ON_ERROR(hr, "GetMixFormat failed");

		int nBlockAlign = 0;
		// coerce int-XX wave format (like int-16 or int-32)
		// can do this in-place since we're not changing the size of the format
		// also, the engine will auto-convert from float to int for us
		if(pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE){
			EXIT_ON_ERROR(E_UNEXPECTED, "Unsupported Wave format");
		}

		// naked scope for case-local variable
		PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pwfx);
		if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat)) {
			// WE GET HERE!
			pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
			// convert it to PCM, but let it keep as many bits of precision as it has initially...though it always seems to be 32
			// comment this out and set wBitsPerSample to  pwfex->wBitsPerSample = getBitsPerSample(); to get an arguably "better" quality 32 bit pcm
			// unfortunately flash media live encoder basically rejects 32 bit pcm, and it's not a huge gain sound quality-wise, so disabled for now.
			pwfx->wBitsPerSample = 16;
			pEx->Samples.wValidBitsPerSample = pwfx->wBitsPerSample;
			pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
			pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
			nBlockAlign = pwfx->nBlockAlign;
			// see also setupPwfex method
		}else {
			EXIT_ON_ERROR(E_UNEXPECTED, "Invalid Wave format");
		}

		LINFO(L"ws=%d, nSamplesPerSec=%d, wBitsPerSample=%d, nChannels=%d", ws, pwfx->nSamplesPerSec, pwfx->wBitsPerSample, pwfx->nChannels);
		if (ws == Wave_In) {
			m_ap_i.SetOrgAudioParam(AF_PCM, pwfx->nSamplesPerSec, pwfx->wBitsPerSample, pwfx->nChannels);
		}else{
			m_ap_o.SetOrgAudioParam(AF_PCM, pwfx->nSamplesPerSec, pwfx->wBitsPerSample, pwfx->nChannels);
		}

		DWORD StreamFlags = 0;
		if (ws == Wave_Out) {
			StreamFlags |= AUDCLNT_STREAMFLAGS_LOOPBACK;
		}

		REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
		hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,StreamFlags,hnsRequestedDuration,0,pwfx,NULL);
		EXIT_ON_ERROR(hr, "AudioClient Initialize failed");

		// Get the size of the allocated buffer.
		UINT32 bufferFrameCount;
		hr = pAudioClient->GetBufferSize(&bufferFrameCount);
		EXIT_ON_ERROR(hr, "AudioClient GetBufferSize failed");

		hr = pAudioClient->GetService(IID_IAudioCaptureClient,(void**)&pCaptureClient);
		EXIT_ON_ERROR(hr, "AudioClient GetService failed");


		// Calculate the actual duration of the allocated buffer.
		REFERENCE_TIME hnsActualDuration = (double)REFTIMES_PER_SEC * bufferFrameCount / pwfx->nSamplesPerSec;

		hr = pAudioClient->Start();  // Start recording.
		EXIT_ON_ERROR(hr, "AudioClient Start failed");

		// Each loop fills about half of the shared buffer.
		while (m_running)
		{
			// Sleep for half the buffer duration.
			Sleep(hnsActualDuration / REFTIMES_PER_MILLISEC / 2);

			UINT32 packetLength = 0;
			hr = pCaptureClient->GetNextPacketSize(&packetLength);
			EXIT_ON_ERROR(hr, "GetNextPacketSize failed");

			while (m_running && packetLength != 0)
			{
				// Get the available data in the shared buffer.
				BYTE *pData;
				DWORD flags;
				UINT32 numFramesAvailable;
				hr = pCaptureClient->GetBuffer(&pData,&numFramesAvailable,&flags, NULL, NULL);
				EXIT_ON_ERROR(hr, "GetBuffer failed");

				if (flags & AUDCLNT_BUFFERFLAGS_SILENT){
					pData = NULL;  // Tell CopyData to write silence.
				}

				// Copy the available capture data to the audio sink.
				DWORD now = ::GetTickCount();
				if(ws == Wave_In){
					m_ap_i.OnAudioData(pData, nBlockAlign *numFramesAvailable);
					if( now - m_last_time_i > 200 ){
						//this->AddEvent(RecordEvent{EVT_IN_AUDIO,"" });
						m_last_time_i = now;
					}	
				}else if(ws == Wave_Out){
					m_ap_o.OnAudioData(pData, nBlockAlign *numFramesAvailable);
					if( now - m_last_time_o > 200 ){
						//this->AddEvent(RecordEvent{EVT_OUT_AUDIO, ""});
						m_last_time_o = now;
					}	
				}

				hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
				EXIT_ON_ERROR(hr, "ReleaseBuffer failed");

				hr = pCaptureClient->GetNextPacketSize(&packetLength);
				EXIT_ON_ERROR(hr, "GetNextPacketSize1 failed");
			}
		}

		hr = pAudioClient->Stop();  // Stop recording.
		EXIT_ON_ERROR(hr, "AudioClient Stop failed");

		if(bCoInitializeExSucc){
			if(pwfx) CoTaskMemFree(pwfx);
			SAFE_RELEASE(pEnumerator)
			SAFE_RELEASE(pDevice)
			SAFE_RELEASE(pAudioClient)
			SAFE_RELEASE(pCaptureClient)
			CoUninitialize();
		}
	}

	__except(EXCEPTION_EXECUTE_HANDLER ){
		bCrashed = true;
		std::ostringstream stream;
		stream << "crashed, exception code = 0x" << hex << GetExceptionCode();
		strErrMsg = stream.str();
	}

Exit:
	if(bHasError || bCrashed){
		if(nRetryCnt < 3){
			nRetryCnt ++;

			bHasError = false;
			bCrashed = false;
			bCoInitializeExSucc = false;
			pEnumerator = NULL;
			pDevice = NULL;
			pAudioClient = NULL;
			pwfx = NULL;
			pCaptureClient = NULL;

			Sleep(100);

			LINFO(L"retry: %d\n", nRetryCnt);
			goto Recording;
		}

		//this->AddEvent( RecordEvent{EVT_ERROR, strErrMsg} );
	}
}

void Record::RunSimulate() {
	LINFO(L"[%ld]Record::RunSimulate\n", GetCurrentThreadId());

	while (m_running) {
		std::vector<BYTE> audio_data;
		m_ap_i.GetAudioData(audio_data);
		m_ap_o.GetAudioData(audio_data);

		Sleep(20);
	}
}