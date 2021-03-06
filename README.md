# Soundengine

A C++ addon for node.js that allows interacting with soundcards to e.g. play, record or process live microphone samples and send them back to the speakers.
The module uses [PortAudio](http://portaudio.com) to interact with the different host apis.

## Prerequisites

Soundengine requires the shared portaudio library.  

### MacOS (via homebrew)

```sh
$ brew install portaudio fftw
```

### Ubuntu (with apt-get)


```sh
$ sudo apt-get install libasound-dev libportaudio2 portaudio19-dev libfftw3-3 libfftw3-dev libfftw3-double3
```

<!--
Download PortAudio from [here](http://www.portaudio.com/download.html) and unpack it.  
In the unpacked portaudio folder

```sh
$ ./configure && make
$ sudo make install
```
-->

### Windows

...not supported yet.


## Installation

```sh
$ npm i soundengine
```

## Basic usage example

```javascript
const soundengine = require('soundengine')

// Start live transmission from the default input device to the default output device at 22kHz
var engine = new soundengine.engine({sampleRate: 22050})

// Start recording
engine.startRecording()

// Apply a beep to the output when recording has stopped
engine.on('recording_stopped', () => {
    engine.beep({frequency: 300})
})

// Stop recording after 5 seconds
setTimeout(() => {
    // Stop the recording
    engine.stopRecording()

    // Playback of the recording
    engine.startPlayback()
}, 5000)

// Let the programm run forever
process.stdin.resume()
```

## Api

### Devices

To list the available devices

```javascript
const soundengine = require('soundengine')
var devices = soundengine.getDevices()
```

### Device properties

### Engine options

Property                 | Type      | Description
-------------------------|-----------|-------------------------------------------------------
id                       | number    | The device id to use in the engine options.
name                     | string    | The device name.
defaultSampleRate        | number    | The default sample rate of the device.
maxInputChannels         | number    | The maximum supported input channels for this device.
maxOutputChannels        | number    | The maximum supported output channels for this device.
defaultLowInputLatency   | number    | [See PortAudio docs](http://www.portaudio.com/docs/latency.html)
defaultLowOutputLatency  | number    | [See PortAudio docs](http://www.portaudio.com/docs/latency.html)
defaultHighInputLatency  | number    | [See PortAudio docs](http://www.portaudio.com/docs/latency.html)
defaultHighOutputLatency | number    | [See PortAudio docs](http://www.portaudio.com/docs/latency.html)

### Engine methods

The engine class actually has almost all the methods of a nodejs [EventEmitter](https://nodejs.org/api/events.html#events_class_eventemitter) to interact with the upcomming events. Furthermore these methods exist:

* `loadRecording(file: string)` - Loads a wave `file` that is then playable with `startPlayback()`. <sup>(1)</sup>
* `startPlayback()` - Starts playback of the last recording or loaded file.
* `stopPlayback()` - Stops playback.
* `pausePlayback()` - Pauses playback.
* `isPlaying(): boolean` - Returns if playback is active.
* `startRecording()` - Starts recording.
* `stopRecording()` - Stops recording.
* `deleteRecording()` - Deletes the recording that is currently held in memory.
* `saveRecording(file: string)` - Saves the recording to a wave `file`. <sup>(2)</sup>
* `isRecording(): boolean` - Returns if recording is active.
* `getRecordingSamples(): number` - Return the number of total samples.
* `getPlaybackPosition(): number` - Returns the current sample index of the playback.
* `getRecordingSampleAt(index: number): number` - Returns a specific sample (between -1..1) at `index`.
* `getPlaybackProgress(): number` - Returns the relative playback progress (between 0..1).
* `setPlaybackProgress(progress: number)` - Sets the relative playback progress (between 0..1).
* `beep(options?: beepOptions)` - Applies a beep to the output.
* `getVolume(): number` - Returns the volume (between 0..1).
* `setVolume(volume: number)` - Sets the volume (between 0..1).
* `getMute(): boolean` - Returns if the output is muted or not.
* `setMute(mute?: boolean)` - Mutes or unmutes the output.
* `getOptions(): engineOptions` - Returns the current engine options.
* `setOptions(options?: engineOptions)` - Sets the engine options.
* `synchronize()` - Clears the internal buffer queues. If there for example is a large delay between the input and the output after initializing a new engine, calling `synchronize` could potentially minimize this delay.

***Notes:***<br>
*(1) Currently only 32bit floating point waves with the same samplerate of the current engine can be loaded (and the header will not be checked).*<br>
*(2) The wave files are 32bit floating point.*

### Engine options

Option          | Type      | Default                     | Description
----------------|-----------|-----------------------------|------------
sampleRate      | number    | 44100                       | Samples per second for each channel.
bufferSize      | number    | 1024                        | The count of samples for each processing iteration.
inputChannels   | number    | 1                           | The number of input channels.
outputChannels  | number    | 1                           | The number of output channels (should equal inputChannels).
inputDevice     | number    | default input device        | The id of the input device to use or -1 for a output only stream.
outputDevice    | number    | default output device       | The id of the output device to use or -1 for a input only stream.
inputLatency    | number    | default high input latency  | [See PortAudio docs](http://www.portaudio.com/docs/latency.html)
outputLatency   | number    | default high output latency | [See PortAudio docs](http://www.portaudio.com/docs/latency.html)

## Beep options

Option          | Type      | Default               | Description
----------------|-----------|-----------------------|------------
duration        | number    | 200                   | The duration of the applied beep in ms.
frequency       | number    | 700                   | The frequency of the beep (sine wave).
level           | number    | 1.0                   | The volume of the beep (between 0..1).

## Engine events

EventName            | Signature                            | Description
---------------------|--------------------------------------|------------
data                 | (inputBuffer: number[]): number[]    | Will be called when a new `inputBuffer` is available to be processed and returned. **Note: If the processing function takes to long to process the buffer you might experience dropouts.**
info                 | ({min: number[], max: number[]})     | This event gets fired with messurements of the inputBuffer such as peaks (min) for every channel.
playback_started     |                                      | Gets fired when playback started.
playback_stopped     |                                      | Gets fired when playback stopped.
playback_paused      |                                      | Gets fired when playback paused.
playback_progress    | (progress: number)                   | Gets fired when playback progressed with the relative progress.
playback_finished    |                                      | Gets fired when playback reached the end of the recording or loaded wave.
recording_loaded     |                                      | Gets fired when the `loadRecording` loaded a file successfully.
recording_started    |                                      | Gets fired when recording started.
recording_stopped    |                                      | Gets fired when recording stopped.
recording_progress   |                                      | Gets fired when recording progressed.
recording_saved      |                                      | Gets fired when the recording was saved with `saveRecording`.
recording_deleted    |                                      | Gets fired when the recording in memory was deleted.
beep_started         |                                      | Gets fired when the `beep` method started apply a beep to the output.
beep_stopped         |                                      | Gets fired when the `beep` method stopped apply a beep to the output.


## Todos

* Implement fft stuff