#ifndef _RECORD_H
#define _RECORD_H

#include <string>
#include <vector>
#include "audio_processor.h"

class Record {
	public:
		static Record& GetInstance() {
			static Record inst;
			return inst;
		}

		void Start(std::string audio_format="pcm", int sample_rate = 8000, int sample_bits = 16, int channel = 1);
		void Stop();
		void Run(WaveSource ws);
		void RunTest();
		void RunSimulate();
	private:
		std::string GetCurrentTime();
		Record();
		~Record();
		
private:
	bool m_running = false;
	std::thread m_record_thread_i;
	std::thread m_record_thread_o;
	std::thread m_simulate_using_thread;
	AudioProcessor m_ap_i;
	AudioProcessor m_ap_o;

	DWORD m_last_time_i;
	DWORD m_last_time_o;
	std::string m_prefix;
};

#endif
