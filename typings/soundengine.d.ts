declare namespace soundengine {
	export interface engineOptions {
		sampleRate?: number
		bufferSize?: number
		inputChannels?: number
		outputChannels?: number
		inputDevice?: number
		outputDevice?: number
		inputLatency?: number
		outputLatency?: number
		fftWindowSize?: number
		fftOverlapSize?: number
		fftWindowFunction?: string
	}

	export interface beepOptions {
		duration?: number
		frequency?: number
		level?: number
	}

	export class engine {
		new(options?: engineOptions)
		
		addListener(eventName: string, listener: Function)
		eventNames(): string[]
		listenerCount(eventName: string): number
		on(eventName: string, listener: Function)
		once(eventName: string, listener: Function)
		prependListener(eventName: string, listener: Function)
		prependOnceListener(eventName: string, listener: Function)
		removeAllListeners(eventName?: string)
		removeListener(eventName: string, listener: Function)

		loadRecording(file: string)
		startPlayback()
		stopPlayback()
		pausePlayback()
		isPlaying(): boolean

		startRecording()
		stopRecording()
		deleteRecording()
		saveRecording(file: string)
		isRecording(): boolean

		getRecordingSamples(): number
		getPlaybackPosition(): number

		getRecordingSampleAt(index: number): number

		getPlaybackProgress(): number
		setPlaybackProgress(progress: number)

		beep(options?: beepOptions)

		getVolume(): number
		setVolume(volume: number)
		mute(mute?: boolean)
		getMute(): boolean

		getOptions(): engineOptions
		setOptions(options?: engineOptions)

		synchronize()
	}

	export interface Device {
		id: number
		name: string
		hostApi: number
		maxInputChannels: number
		maxOutputChannels: number
		defaultSampleRate: number
		defaultLowInputLatency: number
		defaultLowOutputLatency: number
		defaultHighInputLatency: number
		defaultHighOutputLatency: number
	}

	export function getDevices(): Device[]
}