const soundengine = require('./')

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