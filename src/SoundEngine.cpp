/**
 * @author Martin Mende https://github.com/mmende
 */

#include "SoundEngine.h"

using namespace std;
using namespace v8;

Sound::Engine::Engine() {
	listeners = map<string, vector<Listener*>*>();
	listeners[string("data")] = new vector<Listener*>();
	listeners[string("info")] = new vector<Listener*>();
	listeners[string("fft")] = new vector<Listener*>();

	listeners[string("playback_started")] = new vector<Listener*>();
	listeners[string("playback_stopped")] = new vector<Listener*>();
	listeners[string("playback_paused")] = new vector<Listener*>();
	listeners[string("playback_progress")] = new vector<Listener*>();
	listeners[string("playback_finished")] = new vector<Listener*>();

	listeners[string("recording_loaded")] = new vector<Listener*>();
	listeners[string("recording_started")] = new vector<Listener*>();
	listeners[string("recording_stopped")] = new vector<Listener*>();
	listeners[string("recording_progress")] = new vector<Listener*>();
	listeners[string("recording_saved")] = new vector<Listener*>();
	listeners[string("recording_deleted")] = new vector<Listener*>();
	
	listeners[string("beep_started")] = new vector<Listener*>();
	listeners[string("beep_stopped")] = new vector<Listener*>();

	// Initialize PortAudio (to fetch default devices etc. ...)
	PaError paErr = Pa_Initialize();
	if (paErr != paNoError) {
		Nan::ThrowError(Pa_GetErrorText(paErr));
		return;
	}

	printf("PortAudio %s\n", Pa_GetVersionText());

	// Set default options for PortAudio
	sampleRate = 44100;
	bufferSize = 1024;
	inputChannels = 1;
	outputChannels = 1;
	inputDevice = Pa_GetDefaultInputDevice();
	outputDevice = Pa_GetDefaultOutputDevice();

	// Set default options for fft
	fftWindowSize = 1024;
	fftWindowFunctionType = Square;
	fftWindowFunction = new WindowFunction(fftWindowFunctionType, fftWindowSize);

	// Create the buffer caches
	inBufferCache = vector<float*>();
	outBufferCache = vector<float*>();
	recordingBufferCache = vector<float*>();

	int uvErr;
	uvErr = uv_mutex_init(&inBufferCacheMutex);
	if (uvErr != 0) {
		Nan::ThrowError("Could not create inBufferCache mutex.");
		return;
	}
	uvErr = uv_mutex_init(&outBufferCacheMutex);
	if (uvErr != 0) {
		Nan::ThrowError("Could not create outBufferCache mutex.");
		return;
	}

	// Start the looping thread that reads and writes the audio buffers
	uvErr = uv_mutex_init(&streamMutex);
	if (uvErr != 0) {
		Nan::ThrowError("Could not create streaming mutex.");
		return;
	}

	// Configure PortAudio
	stream = NULL;
	_configureStream();

	uv_timer_init(uv_default_loop(), &processing_timer);
	processing_timer.data = this;
	uv_timer_start(&processing_timer, _processing, 0, 10);

	uv_timer_init(uv_default_loop(), &beep_timer);
	beep_timer.data = this;
}

Sound::Engine::~Engine() {
	// Remove all listeners from all eventNames
	for (map<string, vector<Listener*>*>::iterator it = listeners.begin(); it != listeners.end(); ++it) {
		vector<Listener*>* eventListeners = it->second;
		for (vector<Listener*>::iterator it2 = eventListeners->begin() ; it2 != eventListeners->end(); ++it2) {
			Listener* lsnr = *it2;
			delete lsnr;
		}
		// Delete the eventListeners vector
		delete eventListeners;
	}

	// Destroy the window function
	delete fftWindowFunction;

	// Stop the stream
	_stopStream();
	_destroyStream();
}

void Sound::Engine::Init(Handle<Object> target) {
	// Create the engine class that will be exposed to the soundengine object
	Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
	tpl->SetClassName(Nan::New("engine").ToLocalChecked());
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add the methods to the engine class
	Nan::SetPrototypeMethod(tpl, "addListener", AddListener);
	// -emit(eventName[, ...args])
	Nan::SetPrototypeMethod(tpl, "eventNames", EventNames);
	// -getMaxListeners()
	Nan::SetPrototypeMethod(tpl, "listenerCount", ListenerCount);
	// -listeners(eventName)
	Nan::SetPrototypeMethod(tpl, "on", AddListener);
	Nan::SetPrototypeMethod(tpl, "once", Once);
	Nan::SetPrototypeMethod(tpl, "prependListener", PrependListener);
	Nan::SetPrototypeMethod(tpl, "prependOnceListener", PrependOnceListener);
	Nan::SetPrototypeMethod(tpl, "removeAllListeners", RemoveAllListeners);
	Nan::SetPrototypeMethod(tpl, "removeListener", RemoveListener);


	Nan::SetPrototypeMethod(tpl, "loadRecording", LoadRecording);
	Nan::SetPrototypeMethod(tpl, "startPlayback", StartPlayback);
	Nan::SetPrototypeMethod(tpl, "stopPlayback", StopPlayback);
	Nan::SetPrototypeMethod(tpl, "pausePlayback", PausePlayback);
	Nan::SetPrototypeMethod(tpl, "isPlaying", IsPlaying);
	
	Nan::SetPrototypeMethod(tpl, "startRecording", StartRecording);
	Nan::SetPrototypeMethod(tpl, "stopRecording", StopRecording);
	Nan::SetPrototypeMethod(tpl, "deleteRecording", DeleteRecording);
	Nan::SetPrototypeMethod(tpl, "saveRecording", SaveRecording);
	Nan::SetPrototypeMethod(tpl, "isRecording", IsRecording);

	Nan::SetPrototypeMethod(tpl, "getRecordingSamples", GetRecordingSamples);
	Nan::SetPrototypeMethod(tpl, "getRecordingSampleAt", GetRecordingSampleAt);
	Nan::SetPrototypeMethod(tpl, "getPlaybackProgress", GetPlaybackProgress);

	Nan::SetPrototypeMethod(tpl, "getPlaybackPosition", GetPlaybackPosition);
	Nan::SetPrototypeMethod(tpl, "setPlaybackProgress", SetPlaybackProgress);

	Nan::SetPrototypeMethod(tpl, "beep", Beep);

	Nan::SetPrototypeMethod(tpl, "getVolume", GetVolume);
	Nan::SetPrototypeMethod(tpl, "setVolume", SetVolume);
	Nan::SetPrototypeMethod(tpl, "getMute", GetMute);
	Nan::SetPrototypeMethod(tpl, "setMute", SetMute);

	Nan::SetPrototypeMethod(tpl, "getOptions", GetOptions);
	Nan::SetPrototypeMethod(tpl, "setOptions", SetOptions);

	constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

	// Expose engine to the module (module.exports.engine = ...)
	Nan::Set(target, Nan::New("engine").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
}

void Sound::Engine::New(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	if (info.IsConstructCall()) {
		Engine *engine = new Engine();
		engine->Wrap(info.This());

		if (info[0]->IsObject()) {
			Local<Object> options = Nan::To<Object>(info[0]).ToLocalChecked();
			Nan::Persistent<Object>* opts = new Nan::Persistent<Object>(options);
			engine->_setOptions(opts);
		}
		info.GetReturnValue().Set(info.This());

	} else {
		const int argc = 1;
		Local<Value> argv[argc] = {info[0]};
		Local<Function> cons = Nan::New(constructor());
		info.GetReturnValue().Set(cons->NewInstance(argc, argv));
	}
}

void Sound::Engine::AddListener(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	// Check and get eventName
	if (info.Length() < 1 || info[0]->IsString() == false) {
		Nan::ThrowTypeError("First argument must be an event name.");
		return;
	}
	Local<String> _eventName = Nan::To<String>(info[0]).ToLocalChecked();
	string eventName = string((char*)(*String::Utf8Value(_eventName)));

	// Check and get the listener
	if (info.Length() < 2 || info[1]->IsFunction() == false) {
		Nan::ThrowTypeError("Second argument must be a callback function.");
		return;
	}

	Local<Function> _cb = Local<Function>::Cast(info[1]);
	Nan::Persistent<Function>* cb = new Nan::Persistent<Function>();
	cb->Reset(_cb);

	engine->_addListener(eventName, cb);
}

void Sound::Engine::Once(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	// Check and get eventName
	if (info.Length() < 1 || info[0]->IsString() == false) {
		Nan::ThrowTypeError("First argument must be an event name.");
		return;
	}
	Local<String> _eventName = Nan::To<String>(info[0]).ToLocalChecked();
	string eventName = string((char*)(*String::Utf8Value(_eventName)));

	// Check and get the listener
	if (info.Length() < 2 || info[1]->IsFunction() == false) {
		Nan::ThrowTypeError("Second argument must be a callback function.");
		return;
	}

	Local<Function> _cb = Local<Function>::Cast(info[1]);
	Nan::Persistent<Function>* cb = new Nan::Persistent<Function>();
	cb->Reset(_cb);

	engine->_addListener(eventName, cb, false, true);
}

void Sound::Engine::PrependListener(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	// Check and get eventName
	if (info.Length() < 1 || info[0]->IsString() == false) {
		Nan::ThrowTypeError("First argument must be an event name.");
		return;
	}
	Local<String> _eventName = Nan::To<String>(info[0]).ToLocalChecked();
	string eventName = string((char*)(*String::Utf8Value(_eventName)));

	// Check and get the listener
	if (info.Length() < 2 || info[1]->IsFunction() == false) {
		Nan::ThrowTypeError("Second argument must be a callback function.");
		return;
	}

	Local<Function> _cb = Local<Function>::Cast(info[1]);
	Nan::Persistent<Function>* cb = new Nan::Persistent<Function>();
	cb->Reset(_cb);

	engine->_addListener(eventName, cb, true);
}

void Sound::Engine::PrependOnceListener(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	// Check and get eventName
	if (info.Length() < 1 || info[0]->IsString() == false) {
		Nan::ThrowTypeError("First argument must be an event name.");
		return;
	}
	Local<String> _eventName = Nan::To<String>(info[0]).ToLocalChecked();
	string eventName = string((char*)(*String::Utf8Value(_eventName)));

	// Check and get the listener
	if (info.Length() < 2 || info[1]->IsFunction() == false) {
		Nan::ThrowTypeError("Second argument must be a callback function.");
		return;
	}

	Local<Function> _cb = Local<Function>::Cast(info[1]);
	Nan::Persistent<Function>* cb = new Nan::Persistent<Function>();
	cb->Reset(_cb);

	engine->_addListener(eventName, cb, true, true);
}

void Sound::Engine::EventNames(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());
	// Create an array for the event names
	Local<Array> eventNames = Nan::New<Array>(engine->listeners.size());
	int i = 0;
	//for (map<string, vector<Nan::Persistent<Function>*>*>::iterator it = engine->listeners.begin(); it != engine->listeners.end(); ++it, ++i) {
	for (map<string, vector<Listener*>*>::iterator it = engine->listeners.begin(); it != engine->listeners.end(); ++it, ++i) {
		string eventName = it->first;
		eventNames->Set(i, Nan::New<String>(eventName).ToLocalChecked());
	}
	info.GetReturnValue().Set(eventNames);
}

void Sound::Engine::ListenerCount(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	// Check and get eventName
	if (info.Length() < 1 || info[0]->IsString() == false) {
		Nan::ThrowTypeError("First argument must be an event name.");
		return;
	}
	Local<String> _eventName = Nan::To<String>(info[0]).ToLocalChecked();
	string eventName = string((char*)(*String::Utf8Value(_eventName)));

	int listenerCount = 0;

	// Find all listeners for the eventName
	//map<string, vector<Nan::Persistent<Function>*>*>::iterator it = engine->listeners.find(eventName);
	map<string, vector<Listener*>*>::iterator it = engine->listeners.find(eventName);
	if (it != engine->listeners.end()) {
		//vector<Nan::Persistent<Function>*>* eventListeners = it->second;
		vector<Listener*>* eventListeners = it->second;
		listenerCount = eventListeners->size();
	}
	info.GetReturnValue().Set(Nan::New<Number>(listenerCount));
}

void Sound::Engine::RemoveAllListeners(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	// Check and get eventName
	if (info.Length() < 1 || info[0]->IsString() == false) {
		// Remove all listeners from all eventNames
		for (map<string, vector<Listener*>*>::iterator it = engine->listeners.begin(); it != engine->listeners.end(); ++it) {
			vector<Listener*>* eventListeners = it->second;
			for (vector<Listener*>::iterator it2 = eventListeners->begin() ; it2 != eventListeners->end(); ++it2) {
				Listener* lsnr = *it2;
				delete lsnr;
			}
			// Clear the vector
			eventListeners->clear();
		}
		return;
	}

	// Remove listeners from the specified eventName
	Local<String> _eventName = Nan::To<String>(info[0]).ToLocalChecked();
	string eventName = string((char*)(*String::Utf8Value(_eventName)));
	// Find all listeners for the eventName
	map<string, vector<Listener*>*>::iterator it = engine->listeners.find(eventName);
	if (it != engine->listeners.end()) {
		vector<Listener*>* eventListeners = it->second;
		for (vector<Listener*>::iterator it2 = eventListeners->begin(); it2 != eventListeners->end(); ++it2) {
			Listener* lsnr = *it2;
			delete lsnr;
		}
		// Clear the vector
		eventListeners->clear();
	}
}

void Sound::Engine::RemoveListener(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	// Check and get eventName
	if (info.Length() < 1 || info[0]->IsString() == false) {
		Nan::ThrowTypeError("First argument must be an event name.");
		return;
	}
	Local<String> _eventName = Nan::To<String>(info[0]).ToLocalChecked();
	string eventName = string((char*)(*String::Utf8Value(_eventName)));

	// Check and get the listener
	if (info.Length() < 2 || info[1]->IsFunction() == false) {
		Nan::ThrowTypeError("Second argument must be a callback function.");
		return;
	}

	Local<Function> listener = Local<Function>::Cast(info[1]);

	// Find all listeners for the eventName
	map<string, vector<Listener*>*>::iterator it = engine->listeners.find(eventName);
	if (it != engine->listeners.end()) {
		vector<Listener*>* eventListeners = it->second;
		
		for (vector<Listener*>::iterator it2 = eventListeners->begin(); it2 != eventListeners->end(); ++it2) {
			Listener* lsnr = *it2;
			Nan::Persistent<Function>* _cb = lsnr->cb;
			Local<Function> storedListener = Nan::New(*_cb);
			if (storedListener == listener) {
				delete lsnr;
				// Remove it from the list
				eventListeners->erase(it2);
				return;
			}
		}
	}
}

void Sound::Engine::LoadRecording(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	if (info.Length() < 1 || info[0]->IsString() == false) {
		Nan::ThrowTypeError("First argument must be a wave filename.");
		return;
	}
	Local<String> _file = Nan::To<String>(info[0]).ToLocalChecked();
	string file = string((*String::Utf8Value(_file)));
	engine->_loadWave(file);
}

void Sound::Engine::StartPlayback(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	if (engine->isRecording) {
		printf("Cannot start playback while recording.\n");
		return;
	}

	engine->isPlaying = true;
	engine->_emit("playback_started", 0, {});
}

void Sound::Engine::StopPlayback(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	engine->isPlaying = false;
	engine->playbackBufferCacheIdx = 0;
	engine->_emit("playback_stopped", 0, {});
}

void Sound::Engine::PausePlayback(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	engine->isPlaying = false;
	engine->_emit("playback_paused", 0, {});
}

void Sound::Engine::IsPlaying(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	info.GetReturnValue().Set(Nan::New<Boolean>(engine->isPlaying));
}

void Sound::Engine::StartRecording(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	if (engine->isPlaying) {
		printf("Cannot start recording since playback is active...\n");
		return;
	}

	if (engine->isRecording) {
		printf("Already recording...\n");
		return;
	}

	// Clear the recording buffer
	engine->_deleteRecording();

	engine->isRecording = true;
	engine->_emit("recording_started", 0, {});
}

void Sound::Engine::StopRecording(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	if (engine->isRecording == false) {
		printf("Recording never started...\n");
		return;
	}

	engine->isRecording = false;
	engine->_emit("recording_stopped", 0, {});
}

void Sound::Engine::DeleteRecording(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());
	engine->_deleteRecording();
}

void Sound::Engine::SaveRecording(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	if (info.Length() < 1 || info[0]->IsString() == false) {
		Nan::ThrowTypeError("First argument must be a wave filename.");
		return;
	}
	Local<String> _file = Nan::To<String>(info[0]).ToLocalChecked();
	string file = string((*String::Utf8Value(_file)));
	engine->_saveRecording(file);
}

void Sound::Engine::IsRecording(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());
	info.GetReturnValue().Set(Nan::New<Boolean>(engine->isRecording));
}

void Sound::Engine::GetRecordingSamples(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());
	info.GetReturnValue().Set(Nan::New<Integer>((int)engine->recordingBufferCache.size() * engine->bufferSize));
}

void Sound::Engine::GetRecordingSampleAt(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	if (info.Length() < 1 || info[0]->IsNumber() == false) {
		Nan::ThrowTypeError("First argument must be an index number.");
		return;
	}
	Local<Integer> _idx = Nan::To<Integer>(info[0]).ToLocalChecked();
	int idx = _idx->Int32Value();
	
	int inBufferIdx = idx % engine->bufferSize;
	int bufferIdx = floor(idx / engine->bufferSize);

	if (bufferIdx < 0 || bufferIdx >= (int)engine->recordingBufferCache.size()) {
		printf("Warning: Index %i out of recording range\n", idx);
		info.GetReturnValue().Set(Nan::New<Number>(0.0));
		return;
	}
	float* containingBuffer = engine->recordingBufferCache.at(bufferIdx);
	float sample = containingBuffer[inBufferIdx];
	info.GetReturnValue().Set(Nan::New<Number>(sample));
}

void Sound::Engine::GetPlaybackProgress(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());
	double progress = (double)engine->playbackBufferCacheIdx / (double)engine->recordingBufferCache.size();
	info.GetReturnValue().Set(Nan::New<Number>(progress));
}

void Sound::Engine::SetPlaybackProgress(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	if (info.Length() < 1 || info[0]->IsNumber() == false) {
		Nan::ThrowTypeError("First argument must be a index progress number between 0 and 1.");
		return;
	}
	Local<Number> _progress = Nan::To<Number>(info[0]).ToLocalChecked();
	double progress = _progress->NumberValue();
	if (progress < 0 || progress > 1) {
		Nan::ThrowTypeError("First argument must be a index progress number between 0 and 1.");
		return;
	}

	int newIdx = floor(progress * (double)engine->recordingBufferCache.size());
	newIdx = newIdx < 0 ? 0 : newIdx > (int)engine->recordingBufferCache.size() ? (int)engine->recordingBufferCache.size() : newIdx;
	engine->playbackBufferCacheIdx = newIdx;
}

void Sound::Engine::GetPlaybackPosition(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());
	info.GetReturnValue().Set(Nan::New<Integer>(engine->playbackBufferCacheIdx * engine->bufferSize));
}

void Sound::Engine::Beep(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	if (engine->isBeeping) {
		uv_timer_stop(&(engine->beep_timer));
		engine->isBeeping = false;
		engine->beepIdx = -1;
		engine->beepDuration = BEEP_DETAULT_DURATION;
		engine->beepFrequency = BEEP_DETAULT_FREQUENCY;
		engine->beepLevel = BEEP_DETAULT_LEVEL;
	}

	if (info.Length() >= 1 && info[0]->IsObject()) {
		Local<Object> options = Nan::To<Object>(info[0]).ToLocalChecked();
		if (Nan::HasOwnProperty(options, Nan::New<String>("frequency").ToLocalChecked()).FromMaybe(false)) {
			Local<Integer> _frequency = Nan::To<Integer>(Nan::Get(options, Nan::New<String>("frequency").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
			engine->beepFrequency = (int)_frequency->Int32Value();
		}
		if (Nan::HasOwnProperty(options, Nan::New<String>("level").ToLocalChecked()).FromMaybe(false)) {
			Local<Number> _level = Nan::To<Number>(Nan::Get(options, Nan::New<String>("level").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
			engine->beepLevel = _level->NumberValue();
		}
		if (Nan::HasOwnProperty(options, Nan::New<String>("duration").ToLocalChecked()).FromMaybe(false)) {
			Local<Number> _duration = Nan::To<Number>(Nan::Get(options, Nan::New<String>("duration").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
			engine->beepDuration = _duration->NumberValue();
		}
	}
	// Calculate the last beep index
	engine->beepEndIdx = engine->sampleRate * engine->inputChannels * engine->beepDuration;
	uv_timer_start(&(engine->beep_timer), _stopBeep, engine->beepDuration, 1000); // The 1000 is not relevant since the timer will be stopped on the first call
	engine->isBeeping = true;
}

void Sound::Engine::GetVolume(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());
	Local<Number> _volume = Nan::New<Number>(engine->volume);
	info.GetReturnValue().Set(_volume);
}

void Sound::Engine::SetVolume(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	if (info.Length() < 1 || info[0]->IsNumber() == false) {
		Nan::ThrowTypeError("First argument must be number between 0 and 1.");
		return;
	}
	Local<Number> _volume = Nan::To<Number>(info[0]).ToLocalChecked();
	engine->volume = _volume->NumberValue();
}

void Sound::Engine::GetMute(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());
	Local<Boolean> _mute = Nan::New<Boolean>(engine->isMuted);
	info.GetReturnValue().Set(_mute);
}

void Sound::Engine::SetMute(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	// Default to true
	bool mute = true;
	if (info.Length() >= 1 && info[0]->IsBoolean()) {
		Local<Boolean> _mute = Nan::To<Boolean>(info[0]).ToLocalChecked();
		mute = _mute->BooleanValue();
	}
	engine->isMuted = mute;
}

void Sound::Engine::GetOptions(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	// Create the options object that will be returned
	Local<Object> options = Nan::New<Object>();

	Nan::Set(options, Nan::New<String>("sampleRate").ToLocalChecked(), Nan::New<Integer>(engine->sampleRate));
	Nan::Set(options, Nan::New<String>("bufferSize").ToLocalChecked(), Nan::New<Integer>(engine->bufferSize));
	Nan::Set(options, Nan::New<String>("inputChannels").ToLocalChecked(), Nan::New<Integer>(engine->inputChannels));
	Nan::Set(options, Nan::New<String>("outputChannels").ToLocalChecked(), Nan::New<Integer>(engine->outputChannels));
	Nan::Set(options, Nan::New<String>("inputDevice").ToLocalChecked(), Nan::New<Integer>(engine->inputDevice));
	Nan::Set(options, Nan::New<String>("outputDevice").ToLocalChecked(), Nan::New<Integer>(engine->outputDevice));

	Nan::Set(options, Nan::New<String>("fftWindowSize").ToLocalChecked(), Nan::New<Integer>(engine->fftWindowSize));
	Nan::Set(options, Nan::New<String>("fftOverlapSize").ToLocalChecked(), Nan::New<Number>(engine->fftOverlapSize));

	string wftype;
	switch(engine->fftWindowFunctionType) {
		case Square: wftype = "Square"; break;
		case VonHann: wftype = "VonHann"; break;
		case Hamming: wftype = "Hamming"; break;
		case Blackman: wftype = "Blackman"; break;
		case BlackmanHarris: wftype = "BlackmanHarris"; break;
		case BlackmanNuttall: wftype = "BlackmanNuttall"; break;
		case FlatTop: wftype = "FlatTop"; break;
		default: wftype = "unknown";
	}
	Nan::Set(options, Nan::New<String>("fftWindowFunction").ToLocalChecked(), Nan::New<String>(wftype).ToLocalChecked());

	// Return the options object
	info.GetReturnValue().Set(options);
}

void Sound::Engine::SetOptions(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	Nan::HandleScope scope;
	Engine* engine = Nan::ObjectWrap::Unwrap<Engine>(info.Holder());

	// Check and get eventName
	if (info.Length() < 1 || info[0]->IsObject() == false) {
		Nan::ThrowTypeError("First argument must be an options object.");
		return;
	}
	Local<Object> options = Nan::To<Object>(info[0]).ToLocalChecked();
	Nan::Persistent<Object>* opts = new Nan::Persistent<Object>(options);
	engine->_setOptions(opts);
}



void Sound::Engine::_processing(uv_timer_t *handle) {
	Engine* engine = (Engine*)(handle->data);
	Nan::HandleScope scope;

	uv_mutex_lock(&(engine->inBufferCacheMutex));
	float* inputBuffer;
	// Check if a inputBuffer is available
	if (engine->inBufferCache.size() > 0) {
		// Get the inputBuffer and remove the reference from the vector.
		inputBuffer = engine->inBufferCache.front();
		engine->inBufferCache.erase(engine->inBufferCache.begin());
	} else {
		// No data to process...
		uv_mutex_unlock(&(engine->inBufferCacheMutex));
		return;
	}
	uv_mutex_unlock(&(engine->inBufferCacheMutex));

	// Overwrite with playback buffer when isPlaying is active
	if (engine->isPlaying) {
	// Playing back
		if (engine->recordingBufferCache.size() > 0 && engine->playbackBufferCacheIdx < (int)engine->recordingBufferCache.size()) {
			float* playbackBuffer = engine->recordingBufferCache.at(engine->playbackBufferCacheIdx);
			memcpy(inputBuffer, playbackBuffer, engine->bufferSize * sizeof(float));
			Local<Number> progress = Nan::New<Number>((double)engine->playbackBufferCacheIdx/(double)engine->recordingBufferCache.size());
			Local<Value> argv[1] = {progress};
			engine->_emit("playback_progress", 1, argv);
			++engine->playbackBufferCacheIdx;
		} else {
			engine->isPlaying = false;
			engine->playbackBufferCacheIdx = 0;
			engine->_emit("playback_finished", 0, {});
		}
	} else if (engine->isRecording) {
	// Recording
		float* recordingBuffer = new float[engine->bufferSize];
		memcpy(recordingBuffer, inputBuffer, engine->bufferSize * sizeof(float));
		engine->recordingBufferCache.push_back(recordingBuffer);
		// @todo: add progress info
		engine->_emit("recording_progress", 0, {});
	}

	// Calculate peaks etc.
	float min[engine->inputChannels];
	float max[engine->inputChannels];
	int channelIdx, sampleIdx;
	for (channelIdx = 0; channelIdx < engine->inputChannels; ++channelIdx) {
		// The maximum is 1.0
		min[channelIdx] = 1.0;
		// The minimum -1.0
		max[channelIdx] = -1.0;
		for (sampleIdx = 0; sampleIdx < engine->bufferSize; ++sampleIdx) {
			int idx = (channelIdx+1) * sampleIdx;
			float sample = inputBuffer[idx];
			// Calculate min/max values
			if (sample < min[channelIdx]) min[channelIdx] = sample;
			if (sample > max[channelIdx]) max[channelIdx] = sample;
		}
	}

	// Emit the info object
	Local<Object> info = Nan::New<Object>();
	Local<Array> minima = Nan::New<Array>(engine->inputChannels);
	Local<Array> maxima = Nan::New<Array>(engine->inputChannels);
	for (channelIdx = 0; channelIdx < engine->inputChannels; ++channelIdx) {
		minima->Set(channelIdx, Nan::New<Number>(min[channelIdx]));
		maxima->Set(channelIdx, Nan::New<Number>(max[channelIdx]));
	}
	Nan::Set(info, Nan::New<String>("min").ToLocalChecked(), minima);
	Nan::Set(info, Nan::New<String>("max").ToLocalChecked(), maxima);
	Local<Value> argv[] = {info};
	engine->_emit("info", 1, argv);

	// If there are data listeners, let them process the buffer
	map<string, vector<Listener*>*>::iterator it = engine->listeners.find(string("data"));
	if (it != engine->listeners.end()) {

		// Create a v8 array for the processing
		Local<Array> processingBuffer = Nan::New<Array>(engine->bufferSize);
		for (int i = 0; i < engine->bufferSize; ++i) {
			processingBuffer->Set(i, Nan::New<Number>(inputBuffer[i]));
		}
		
		vector<Listener*>* eventListeners = it->second;
		vector<Listener*>::iterator it2 = eventListeners->begin();
		while(it2 != eventListeners->end()) {
			Listener* lsnr = *it2;
			Nan::Persistent<Function>* _cb = lsnr->cb;
			Local<Function> fn = Nan::New<Function>(*_cb);
			Nan::Callback cb(fn);

			int argc = 1;
			Local<Value> argv[1] = {processingBuffer};
			Local<Value> resultBuffer = cb.Call(argc, argv);
			if (resultBuffer->IsArray()) {
				processingBuffer = Local<Array>::Cast(resultBuffer);
			} else {
				Nan::ThrowTypeError("Return type for data listener must be an array.");
			}

			if (lsnr->once) {
				it2 = eventListeners->erase(it2);
				delete lsnr;
			} else {
				++it2;
			}
		}

		// Copy the processed values in the buffer
		for (int i = 0; i < engine->bufferSize; ++i) {
			inputBuffer[i] = Nan::To<double>(processingBuffer->Get(i)).FromJust();
		}
	}

	// Apply outgoing stuff like volume, beep etc.
	for (int i = 0; i < engine->bufferSize; ++i) {
		if (engine->isMuted) {
			inputBuffer[i] = 0;
		} else {
			if (engine->isBeeping) {
				++engine->beepIdx;
				if (engine->beepIdx == 0) engine->_emit("beep_started", 0, {});
				double relPos = (double)engine->beepIdx / (double)engine->beepEndIdx;
				//Math.sin(2 * this.beepFrequency * (position * this.beepTotal * Math.PI)) * 0.72 * this.beepLevel
				inputBuffer[i] += sin((double)2 * engine->beepFrequency * (relPos * engine->beepDuration * M_PI)) * 0.72 * engine->beepLevel;
			}
			inputBuffer[i] *= engine->volume;
		}
	}

	// Add the processed inputBuffer to the outBufferCache
	uv_mutex_lock(&(engine->outBufferCacheMutex));
	engine->outBufferCache.push_back(inputBuffer);
	uv_mutex_unlock(&(engine->outBufferCacheMutex));
}

int Sound::Engine::_streamCallback(
			const void* input, void* output,
			unsigned long frameCount,
			const PaStreamCallbackTimeInfo* timeInfo,
			PaStreamCallbackFlags statusFlags,
			void *userData)
{
	Engine* engine = (Engine*)(userData);
	
	int streamLocked = uv_mutex_trylock(&(engine->streamMutex));
	if(streamLocked != 0) return 0;

	int samplesCount = engine->bufferSize;// * engine->inputChannels;

	float* inputBuffer = (float*)input;
	float* outputBuffer = (float*)output;

	// Copy the inputBuffer onto the cache
	uv_mutex_lock(&(engine->inBufferCacheMutex));
	float* inCopy = new float[samplesCount];
	memcpy(inCopy, inputBuffer, sizeof(float) * samplesCount);
	engine->inBufferCache.push_back(inCopy);
	uv_mutex_unlock(&(engine->inBufferCacheMutex));

	// Get an outputBuffer from the cache
	uv_mutex_lock(&(engine->outBufferCacheMutex));
	if (engine->outBufferCache.size() > 0) {
		float* outCopy = engine->outBufferCache.front();
		memcpy(outputBuffer, outCopy, sizeof(float) * samplesCount);
		delete outCopy;
		engine->outBufferCache.erase(engine->outBufferCache.begin());
	}
	uv_mutex_unlock(&(engine->outBufferCacheMutex));

	uv_mutex_unlock(&(engine->streamMutex));
	return 0;
}

void Sound::Engine::_stopBeep(uv_timer_t *handle) {
	// Stop the timer
	uv_timer_stop(handle);

	Engine* engine = (Engine*)(handle->data);
	Nan::HandleScope scope;

	// Set defaults
	engine->isBeeping = false;
	engine->beepIdx = -1;
	engine->beepDuration = BEEP_DETAULT_DURATION;
	engine->beepFrequency = BEEP_DETAULT_FREQUENCY;
	engine->beepLevel = BEEP_DETAULT_LEVEL;

	engine->_emit("beep_stopped", 0, {});
}



void Sound::Engine::_configureStream() {
	PaError paErr;

	// Apply the options
	inputParameters.device = inputDevice;
	inputParameters.channelCount = inputChannels;
	inputParameters.sampleFormat = paFloat32;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputDevice)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = NULL;
	outputParameters.device = outputDevice;
	outputParameters.channelCount = outputChannels;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputDevice)->defaultHighOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	// Open the stream with the specified parameters
	paErr = Pa_OpenStream(
		&stream,
		&inputParameters,
		&outputParameters,
		sampleRate,
		bufferSize,
		paClipOff,
		_streamCallback,
		this
	);
	if (paErr != paNoError) {
		Nan::ThrowError(Pa_GetErrorText(paErr));
		return;
	}
}

void Sound::Engine::_startStream() {
	PaError paErr = Pa_IsStreamActive(stream);
	if (paErr == 1) {
		printf("Stream already active...\n");
		return;
	}

	paErr = Pa_StartStream(stream);
	if (paErr != paNoError) {
		Nan::ThrowError(Pa_GetErrorText(paErr));
		return;
	}

	// Restart the processing timer
	uv_timer_start(&processing_timer, _processing, 0, 10);
}

void Sound::Engine::_stopStream() {
	// Stop processing
	uv_timer_stop(&processing_timer);

	PaError paErr = Pa_IsStreamStopped(stream);
	if (paErr == 1) {
		printf("Stream already stopped...\n");
		return;
	}

	paErr = Pa_StopStream(stream);
	if (paErr != paNoError) {
		Nan::ThrowError(Pa_GetErrorText(paErr));
		return;
	}

	// Clear caches
	while (!inBufferCache.empty()) {
		delete inBufferCache.back();
		inBufferCache.pop_back();
	}
	while (!outBufferCache.empty()) {
		delete outBufferCache.back();
		outBufferCache.pop_back();
	}
}

void Sound::Engine::_destroyStream() {
	PaError err = Pa_Terminate();
	if (err != paNoError) {
		Nan::ThrowError(Pa_GetErrorText(err));
	}
}

void Sound::Engine::_loadWave(string file) {
	ifstream waveFile(file.c_str(), ios::in | ios::binary);
	if (waveFile) {
		// Get the file size
		waveFile.seekg(0, waveFile.end);
		int fileSize = waveFile.tellg();
		waveFile.seekg(0, waveFile.beg);

		// Check the header first
		char _header[44];
		waveFile.read(_header, 44);
		WaveHeader header = WaveHeader();
		memcpy(&header, _header, 44);
		// @todo: check
		
		// Seek to the data
		waveFile.seekg(44, waveFile.beg);
		char _data[fileSize - 44];
		waveFile.read(_data, fileSize - 44);

		int samplesCount = (fileSize - 44) / sizeof(float);
		float data[samplesCount];
		memcpy(&data, _data, fileSize - 44);

		// Delete the recording buffer cache to store the loaded file into it
		_deleteRecording();

		// Fille the playback cache buffer
		float* playbackBuffer = new float[bufferSize];
		recordingBufferCache.push_back(playbackBuffer);
		for (int i = 0, j = 0; i < samplesCount; ++i) {
			float sample = data[i];
			playbackBuffer[j] = sample;
			++j;
			if (j >= bufferSize) {
				j = 0;
				playbackBuffer = new float[bufferSize];
				recordingBufferCache.push_back(playbackBuffer);
			}
		}
		waveFile.close();

		_emit("recording_loaded", 0, {});
	}
}

void Sound::Engine::_deleteRecording() {
	while (!recordingBufferCache.empty()) {
		delete[] recordingBufferCache.back();
		recordingBufferCache.pop_back();
	}
}

void Sound::Engine::_saveRecording(string file) {
	// Calculate the data size for the header
	int dataSize = bufferSize  * recordingBufferCache.size() * sizeof(float);
	vector<string> data = vector<string>();

	// Copy the recordingData into the buffer
	for (vector<float*>::iterator it = recordingBufferCache.begin(); it != recordingBufferCache.end(); ++it) {
		char charBuffer[bufferSize * sizeof(float)];
		float* floatBuffer = *it;
		memcpy(charBuffer, floatBuffer, bufferSize * sizeof(float));
		data.push_back(string(charBuffer, bufferSize * sizeof(float)));
	}

	// Create the wave header
	char header[44];
	WaveHeader _header = WaveHeader();

	int bitsPerSample = 32;
	short int blockAlign = inputChannels * ((bitsPerSample + 7) / 8);
	int byteRate = sampleRate * (int)blockAlign;

	memcpy(_header.RIFF, "RIFF", 4);
	_header.ChunkSize = dataSize - 8;
	memcpy(_header.WAVE, "WAVE", 4);
	memcpy(_header.fmt, "fmt ", 4);
	_header.Subchunk1Size = 16;
	_header.AudioFormat = 3; // Since we have float data
	_header.NumOfChan = inputChannels;
	_header.SamplesPerSec = sampleRate;
	_header.bytesPerSec = byteRate;
	_header.blockAlign = blockAlign;
	_header.bitsPerSample = bitsPerSample;
	memcpy(_header.data, "data", 4);
	_header.Subchunk2Size = dataSize;
	memcpy(header, &_header, sizeof(_header));

	// Add the header in front of the data
	data.insert(data.begin(), string(header, 44));

	// Write all chunks of data to the file
	FILE* fp;
	fp = fopen(file.c_str(), "wb");
	for (vector<string>::iterator it = data.begin(); it != data.end(); ++it) {
		string chunk = *it;
		int dataChunkSize = chunk.size();
		// Create a char array without null termination
		char* chunkData = new char[dataChunkSize];
		for (int i = 0; i < dataChunkSize; ++i) {
			chunkData[i] = chunk[i];
		}
		fwrite(chunkData, 1, dataChunkSize, fp);
		fflush(fp);
		delete[] chunkData;
	}
	fclose(fp);
}

void Sound::Engine::_addListener(
	string eventName,
	Nan::Persistent<Function>* listener,
	bool prepend,
	bool once)
{
	map<string, vector<Listener*>*>::iterator it = this->listeners.find(eventName);
	if (it != this->listeners.end()) {
		vector<Listener*>* eventListeners = it->second;
		Listener* lsnr = new Listener(listener, once);
		if (prepend == false) {
			// Append the callback
			eventListeners->push_back(lsnr);
		} else {
			// Prepend the callback
			eventListeners->insert(eventListeners->begin(), lsnr);
		}
		return;
	}
	// Dispose the listener because such an eventName does not exist
	listener->Reset();
}

void Sound::Engine::_emit(string eventName, int argc, Local<Value> argv[]) {
	map<string, vector<Listener*>*>::iterator it = listeners.find(eventName);
	if (it != listeners.end()) {
		vector<Listener*>* eventListeners = it->second;
		vector<Listener*>::iterator it2 = eventListeners->begin();
		while(it2 != eventListeners->end()) {
			Listener* lsnr = *it2;
			Nan::Persistent<Function>* _cb = lsnr->cb;
			Local<Function> fn = Nan::New<Function>(*_cb);
			Nan::Callback cb(fn);

			cb.Call(argc, argv);

			if (lsnr->once) {
				it2 = eventListeners->erase(it2);
				delete lsnr;
			} else {
				++it2;
			}
		}
	}
}

void Sound::Engine::_setOptions(Nan::Persistent<Object>* opts) {
	// Get the options object back
	Local<Object> options = Nan::New(*opts);

	if (Nan::HasOwnProperty(options, Nan::New<String>("sampleRate").ToLocalChecked()).FromMaybe(false)) {
		Local<Integer> _sampleRate = Nan::To<Integer>(Nan::Get(options, Nan::New<String>("sampleRate").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
		sampleRate = (int)_sampleRate->Int32Value();
	}

	if (Nan::HasOwnProperty(options, Nan::New<String>("bufferSize").ToLocalChecked()).FromMaybe(false)) {
		Local<Integer> _bufferSize = Nan::To<Integer>(Nan::Get(options, Nan::New<String>("bufferSize").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
		bufferSize = (int)_bufferSize->Int32Value();
	}

	if (Nan::HasOwnProperty(options, Nan::New<String>("inputChannels").ToLocalChecked()).FromMaybe(false)) {
		Local<Integer> _inputChannels = Nan::To<Integer>(Nan::Get(options, Nan::New<String>("inputChannels").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
		inputChannels = (int)_inputChannels->Int32Value();
	}

	if (Nan::HasOwnProperty(options, Nan::New<String>("outputChannels").ToLocalChecked()).FromMaybe(false)) {
		Local<Integer> _outputChannels = Nan::To<Integer>(Nan::Get(options, Nan::New<String>("outputChannels").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
		outputChannels = (int)_outputChannels->Int32Value();
	}

	if (Nan::HasOwnProperty(options, Nan::New<String>("inputDevice").ToLocalChecked()).FromMaybe(false)) {
		Local<Integer> _inputDevice = Nan::To<Integer>(Nan::Get(options, Nan::New<String>("inputDevice").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
		inputDevice = (int)_inputDevice->Int32Value();
	}

	if (Nan::HasOwnProperty(options, Nan::New<String>("outputDevice").ToLocalChecked()).FromMaybe(false)) {
		Local<Integer> _outputDevice = Nan::To<Integer>(Nan::Get(options, Nan::New<String>("outputDevice").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
		outputDevice = (int)_outputDevice->Int32Value();
	}

	if (Nan::HasOwnProperty(options, Nan::New<String>("fftWindowSize").ToLocalChecked()).FromMaybe(false)) {
		Local<Integer> _fftWindowSize = Nan::To<Integer>(Nan::Get(options, Nan::New<String>("fftWindowSize").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
		fftWindowSize = (int)_fftWindowSize->Int32Value();
		// Recreate the window function
		delete fftWindowFunction;
		fftWindowFunction = new WindowFunction(fftWindowFunctionType, fftWindowSize);
	}

	if (Nan::HasOwnProperty(options, Nan::New<String>("fftOverlapSize").ToLocalChecked()).FromMaybe(false)) {
		Local<Number> _fftWindowSize = Nan::To<Number>(Nan::Get(options, Nan::New<String>("fftOverlapSize").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
		fftOverlapSize = (float)_fftWindowSize->NumberValue();
	}

	if (Nan::HasOwnProperty(options, Nan::New<String>("fftWindowFunction").ToLocalChecked()).FromMaybe(false) ) {
		Local<String> _str = Nan::To<String>(Nan::Get(options, Nan::New<String>("fftWindowFunction").ToLocalChecked()).ToLocalChecked()).ToLocalChecked();
		string wftype = string((*String::Utf8Value(_str)));
		const char* desiredWindowFunction = wftype.c_str();
		bool validWindowFunctionType = true;
		if 		(strcmp(desiredWindowFunction, "Square") == 0)			fftWindowFunctionType = Square;
		else if (strcmp(desiredWindowFunction, "VonHann") == 0)			fftWindowFunctionType = VonHann;
		else if (strcmp(desiredWindowFunction, "Hamming") == 0)			fftWindowFunctionType = Hamming;
		else if (strcmp(desiredWindowFunction, "Blackman") == 0) 		fftWindowFunctionType = Blackman;
		else if (strcmp(desiredWindowFunction, "BlackmanHarris") == 0)	fftWindowFunctionType = BlackmanHarris;
		else if (strcmp(desiredWindowFunction, "BlackmanNuttall") == 0)	fftWindowFunctionType = BlackmanNuttall;
		else if (strcmp(desiredWindowFunction, "FlatTop") == 0)			fftWindowFunctionType = FlatTop;
		else {
			printf("Unknown window function %s.\n", desiredWindowFunction);
			validWindowFunctionType = false;
		}

		if (validWindowFunctionType) {
			delete fftWindowFunction;
			fftWindowFunction = new WindowFunction(fftWindowFunctionType, fftWindowSize);
		}
	}

	// The persistent can now be disposed
	opts->Reset();
	delete opts;

	uv_mutex_lock(&streamMutex);
	_stopStream();
	_configureStream();
	_startStream();
	uv_mutex_unlock(&streamMutex);
}



void Sound::GetDevices(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	// PortAudio must be initialized to see the available devices
	PaError paErr = Pa_Initialize();
	if (paErr != paNoError) {
		Nan::ThrowError(Pa_GetErrorText(paErr));
		return;
	}

	// Create an array to hold the devices
	Local<Array> devices = Nan::New<Array>();

	int numDevices = Pa_GetDeviceCount();

	for (int i = 0; i < numDevices; ++i) {
		// Create an object for the device
		Local<Object> device = Nan::New<Object>();
		const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);

		Nan::Set(device, Nan::New<String>("id").ToLocalChecked(), Nan::New<Number>(i));
		Nan::Set(device, Nan::New<String>("name").ToLocalChecked(), Nan::New<String>(deviceInfo->name).ToLocalChecked());
		Nan::Set(device, Nan::New<String>("hostApi").ToLocalChecked(), Nan::New<Number>((int)(deviceInfo->hostApi)));
		Nan::Set(device, Nan::New<String>("maxInputChannels").ToLocalChecked(), Nan::New<Number>((int)(deviceInfo->maxInputChannels)));
		Nan::Set(device, Nan::New<String>("maxOutputChannels").ToLocalChecked(), Nan::New<Number>((int)(deviceInfo->maxOutputChannels)));
		Nan::Set(device, Nan::New<String>("defaultSampleRate").ToLocalChecked(), Nan::New<Number>((int)(deviceInfo->defaultSampleRate)));
		
		Nan::Set(device, Nan::New<String>("defaultLowInputLatency").ToLocalChecked(), Nan::New<Number>((double)(deviceInfo->defaultLowInputLatency)));
		Nan::Set(device, Nan::New<String>("defaultLowOutputLatency").ToLocalChecked(), Nan::New<Number>((double)(deviceInfo->defaultLowOutputLatency)));
		Nan::Set(device, Nan::New<String>("defaultHighInputLatency").ToLocalChecked(), Nan::New<Number>((double)(deviceInfo->defaultHighInputLatency)));
		Nan::Set(device, Nan::New<String>("defaultHighOutputLatency").ToLocalChecked(), Nan::New<Number>((double)(deviceInfo->defaultHighOutputLatency)));
		
		Nan::Set(devices, i, device);
	}

	info.GetReturnValue().Set(devices);

	PaError err = Pa_Terminate();
	if (err != paNoError) {
		Nan::ThrowError(Pa_GetErrorText(err));
	}
}

void Sound::ApplyDamping(const Nan::FunctionCallbackInfo<v8::Value>& info) {
	// Check and get the buffer
	if (info.Length() < 1 || info[0]->IsArray() == false) {
		Nan::ThrowTypeError("First argument must be an array.");
		return;
	}
	Local<Array> inBuffer = Local<Array>::Cast(info[0]);
	Local<Array> outBuffer = Nan::New<Array>(inBuffer->Length());

	// Check and get
	if (info.Length() < 2 || info[1]->IsNumber() == false) {
		Nan::ThrowTypeError("Second argument must be a damping coefficient.");
		return;
	}
	Local<Number> damping = Nan::To<Number>(info[1]).ToLocalChecked();
	double coefficient = (double)damping->NumberValue();
	//printf("Damping factor is %f\n", coefficient);
	
	for (int i = 0; i < (int)(inBuffer->Length()); ++i) {
		Local<Value> _value = inBuffer->Get(i);
		// Check if the buffer is interleaved or not
		if (_value->IsNumber()) {
			// Apply coefficient to every value
			double newVal = (double)(_value->NumberValue()) * coefficient;
			outBuffer->Set(i, Nan::New<Number>(newVal));
		} else if (_value->IsArray()) {
			// Apply coefficient to every value in every inner array
			Local<Array> inInner = Local<Array>::Cast(_value);
			Local<Array> outInner = Nan::New<Array>(inInner->Length());
			for (int j = 0; j < (int)(inInner->Length()); ++j) {
				Local<Value> value = inInner->Get(j);
				double newVal = (double)(value->NumberValue()) * coefficient;
				outInner->Set(j, Nan::New<Number>(newVal));
			}
			outBuffer->Set(i, outInner);
		} else {
			printf("Invalid buffer structure.\n");
			return;
		}
	}
	info.GetReturnValue().Set(outBuffer);
}

void Sound::InitOther(Local<Object> target) {
	// Add device functions
	Nan::Set(target, Nan::New("getDevices").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(GetDevices)).ToLocalChecked());
	Nan::Set(target, Nan::New("applyDamping").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(ApplyDamping)).ToLocalChecked());
}

void Sound::InitAll(Handle<Object> target) {
	Engine::Init(target);
	InitOther(target);
}