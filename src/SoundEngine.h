/**
 * @author Martin Mende https://github.com/mmende
 */

#define BEEP_DETAULT_DURATION 200
#define BEEP_DETAULT_FREQUENCY 700
#define BEEP_DETAULT_LEVEL 1.0
#define PROCESSING_INTERVAL 1

#include <v8.h>
#include <nan.h>
#include <map>
#include <vector>
#include <float.h>
#include <sys/stat.h>
#include <fstream>

#include <portaudio.h>
#include <fftw3.h>

#include "WindowFunction.h"

using namespace std;
using namespace v8;

namespace Sound {

	/**
	 * Struct of a wave header with the exact wave-header structure in memory.
	 */
	struct WaveHeader {
		char		RIFF[4];
		int			ChunkSize;
		char		WAVE[4];
		char		fmt[4];
		int			Subchunk1Size;
		short int	AudioFormat;
		short int	NumOfChan;
		int			SamplesPerSec;
		int			bytesPerSec;
		short int	blockAlign;
		short int	bitsPerSample;
		char 		data[4];
		int			Subchunk2Size;
	};

	/**
	 * Used to store callback functions for the event emitter stuff.
	 */
	struct Listener {
		Listener(Nan::Persistent<Function>* cb, bool once): cb(cb), once(once) {

		}
		~Listener() {
			cb->Reset();
		}
		Nan::Persistent<Function>* cb;
		bool once;
	};

	/**
	 * The actual engine that manages audio i/o etc..
	 */
	class Engine: public Nan::ObjectWrap {
	public:
		static NAN_MODULE_INIT(Init);
	private:
		explicit Engine();
		~Engine();
			
		static NAN_METHOD(New);
		static NAN_METHOD(AddListener);
		static NAN_METHOD(Once);
		static NAN_METHOD(PrependListener);
		static NAN_METHOD(PrependOnceListener);
		static NAN_METHOD(EventNames);
		static NAN_METHOD(ListenerCount);
		static NAN_METHOD(RemoveAllListeners);
		static NAN_METHOD(RemoveListener);
		static NAN_METHOD(LoadRecording);
		static NAN_METHOD(StartPlayback);
		static NAN_METHOD(StopPlayback);
		static NAN_METHOD(PausePlayback);
		static NAN_METHOD(IsPlaying);
		static NAN_METHOD(StartRecording);
		static NAN_METHOD(StopRecording);
		static NAN_METHOD(DeleteRecording);
		static NAN_METHOD(SaveRecording);
		static NAN_METHOD(IsRecording);
		static NAN_METHOD(GetRecordingSamples);
		static NAN_METHOD(GetRecordingSampleAt);
		static NAN_METHOD(GetPlaybackProgress);
		static NAN_METHOD(SetPlaybackProgress);
		static NAN_METHOD(GetPlaybackPosition);
		static NAN_METHOD(Beep);
		static NAN_METHOD(GetVolume);
		static NAN_METHOD(SetVolume);
		static NAN_METHOD(GetMute);
		static NAN_METHOD(SetMute);
		static NAN_METHOD(GetOptions);
		static NAN_METHOD(SetOptions);
		static NAN_METHOD(Synchronize);

		static inline Nan::Persistent<Function> & constructor() {
			static Nan::Persistent<Function> construct;
			return construct;
		}
		static void _processing(uv_timer_t *handle);
		static int _streamCallback(
			const void* input, void* output,
			unsigned long frameCount,
			const PaStreamCallbackTimeInfo* timeInfo,
			PaStreamCallbackFlags statusFlags,
			void *userData);
		static void _stopBeep(uv_timer_t *handle);
		
		void _configureStream();
		void _startStream();
		void _stopStream();
		void _destroyStream();
		void _loadWave(string file);
		void _deleteRecording();
		void _saveRecording(string file);
		
		void _addListener(string eventName, Nan::Persistent<Function>* listener, bool prepend = false, bool once = false);
		void _emit(string eventName, int argc, Local<Value> argv[]);
		void _setOptions(Nan::Persistent<Object>* opts);


		// Holds the event listeners for each eventName
		map<string, vector<Listener*>*> listeners;

		/** The PortAudio stuff **/
		PaStream* stream;
		PaStreamParameters inputParameters;
		PaStreamParameters outputParameters;
		// The stream configuration
		int sampleRate;
		int bufferSize;
		int inputChannels;
		int outputChannels;
		int inputDevice;
		int outputDevice;

		// Holds the unprocessed input buffers comming from the soundcard
		vector<float*> inBufferCache;
		// Holds the processed output buffers that go out to the soundcard
		vector<float*> outBufferCache;
		// Used to prevent simultanious usage of the in/out cache buffers from the stream callback and the processing interval
		uv_mutex_t inBufferCacheMutex;
		uv_mutex_t outBufferCacheMutex;
		// The actual processing interval timer
		uv_timer_t processing_timer;
		// Used to prevent using the buffers in the PortAudio callback while e.g. reconfiguring the stream
		uv_mutex_t streamMutex;
		// The volume coefficient
		double volume = 1.0;
		bool isMuted = false;
		// An indicator if a beep is currently being applied
		bool isBeeping = false;
		// The last sample that got a value applied
		int beepIdx = -1;
		// The last sample that will get a value applied
		int beepEndIdx;
		// The frequency of the beep
		int beepFrequency = BEEP_DETAULT_FREQUENCY;
		// The duration in milliseconds
		double beepDuration = BEEP_DETAULT_DURATION;
		// The volume of the beep from 0..1
		double beepLevel = BEEP_DETAULT_LEVEL;
		// The beep timer
		uv_timer_t beep_timer;
		// Get's filled while recording or playback buffers
		vector<float*> recordingBufferCache;
		// An indicator if recording is active
		bool isRecording = false;
		// An indicator if playback is active
		bool isPlaying = false;
		// The last playback buffer that was send to the outBuffer
		int playbackBufferCacheIdx = 0;

		/** The FFT stuff **/
		int fftWindowSize;
		float fftOverlapSize;
		WindowFunctionType fftWindowFunctionType;
		WindowFunction* fftWindowFunction;
	};

	// The device listing method
	NAN_METHOD(GetDevices);
	NAN_METHOD(ApplyDamping);

	void InitOther(Local<Object> target);
	NAN_MODULE_INIT(InitAll);
}

NODE_MODULE(soundengine, Sound::InitAll);