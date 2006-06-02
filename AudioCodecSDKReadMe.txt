AudioCodecSDK

Contained herein is an example of an audio codec and its accompanying SMAC component.

The project builds four components: an IMA audio decoder (adec), an IMA audio encoder (aenc), and SMAC (Sound Manager Audio Component) components for both (sdec and scom, respectively).

The project should built as is in Project Builder 2.1 or XCode. The resulting AudioCodecSDK.component can be installed in /Library/Audio/Plug-Ins/Components. It contains the same IMA audio codec that's in /System/Library/Components/AudioCodecs.component. It will add the SMAC scom and sdec. You can verify it's installed under QuickTime Pro by checking the Export to QuickTime Movie, Options, (Sound) Settings, Compressor. The standard "IMA 4:1" will change to "Apple IMA Audio."

IMA Audio Codec

Included is both an IMA encoder component and decoder component. It is possible to build one but not the other although the project is set up to build both.

Most of the included files can be used as is. The only files the developer actually needs to create are the codec specific files. In the IMA codec, these files are all the files in the "IMA" folder in the project.

SMAC IMA

SMAC is what an audio codec needs to make it behave like Sound Manager components. Think of SMAC as a wrapper for the codec. The system will then see an scom and sdec as well as an aenc and adec. Applications that use the Sound Manager to interact with audio will now be able to use your audio codec along with applications that interact with Core Audio.

Eight of the files in the SMAC folder in the project will need to be replaced by the developer in order to deal with a new codec. Those eight files are all the ones that have "SMACIMA" in the name.

SMAC was originally developed as a bridge to QuickTime, and as such, familiarity with QuickTime will help considerably.

References

http://developer.apple.com/audio/
http://developer.apple.com/quicktime/

