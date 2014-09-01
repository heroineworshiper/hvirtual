/*************************************************************************/
/*                                                                       */
/*                Centre for Speech Technology Research                  */
/*                     University of Edinburgh, UK                       */
/*                         Copyright (c) 1996                            */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  THE UNIVERSITY OF EDINBURGH AND THE CONTRIBUTORS TO THIS WORK        */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL THE UNIVERSITY OF EDINBURGH NOR THE CONTRIBUTORS BE LIABLE     */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*                       Author :  Brian Foley                           */
/*                                 bfoley@compsoc.nuigalway.ie           */
/*                       Date   :  February 2004                         */
/*=======================================================================*/
#include "EST_unix.h"
#include "EST_cutils.h"
#include "EST_Wave.h"
#include "EST_Option.h"
#include "audioP.h"

#if defined (SUPPORT_MACOSX_AUDIO)

#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioConverter.h>
#include <AudioToolbox/DefaultAudioOutput.h>

int macosx_supported = TRUE;
AudioUnit outau;
AudioConverterRef outconv;

short *wave;
UInt32 wavesize;

// The audio conversion callback is trivial: we just point it at our
// buffer of samples and ask it to convert it at its convenience.
OSStatus conv_inproc(AudioConverterRef inAC, UInt32 *size, void **data, void *indata)
{
    if (wavesize > 0) {
        *size = wavesize;
        *data = wave;
        wavesize = 0;
    } else {
        *size = 0;
    }
    return noErr;
}

// The audio 'rendering' is easy: we get AudioConverterFillBuffer to do all the
// heavy lifting of sample rate conversion, filling out extra channels, changing
// formats, and slicing the result up into nice buffer sized frames. 
OSStatus render_callback(void *inref, AudioUnitRenderActionFlags inflags,
    const AudioTimeStamp *instamp, UInt32 inbus, AudioBuffer *iodata)
{
    UInt32 size = iodata->mDataByteSize;
    OSStatus err;
    
    err = AudioConverterFillBuffer (outconv, conv_inproc, inref, &size, iodata->mData);
    
    if (err != noErr || size == 0) {
        AudioOutputUnitStop(outau);        
    }
    
    return noErr;
}

int play_macosx_wave(EST_Wave &inwave, EST_Option &al)
{
    OSStatus err;
    AudioStreamBasicDescription waveformat, outformat;
    UInt32 size = sizeof(AudioStreamBasicDescription);
    UInt32 running;
    AudioUnitInputCallback input = {
        inputProc: render_callback, inputProcRefCon: NULL
    };
    
    wavesize = inwave.num_samples() * sizeof(short);
    wave = inwave.values().memory();
    
    // Open the default audio output, set it up, and attach
    // our audio 'rendering' callback to it.
    err = OpenDefaultAudioOutput(&outau);
    if (err != noErr) {
        cerr << "Couldn't open default audio ouput." << endl;
        return -1;
    }
    
    err = AudioUnitInitialize(outau);
    if (err != noErr) {
        cerr << "Couldn't initialize default audio ouput." << endl;
        CloseComponent(outau);
        return -1;
    }
    
    err = AudioUnitSetProperty(outau, kAudioUnitProperty_SetInputCallback,
        kAudioUnitScope_Global, 0, &input, sizeof(input));
    if (err != noErr) {
        cerr << "Couldn't set up callback." << endl;
        CloseComponent(outau);
        return -1;
    }

    
    // Set up our converter -- map our sample format
    // onto the hardware output format.
    waveformat.mSampleRate = (Float64) inwave.sample_rate();
    waveformat.mFormatID = kAudioFormatLinearPCM;
    waveformat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger
        | kLinearPCMFormatFlagIsPacked | kLinearPCMFormatFlagIsBigEndian;
    waveformat.mBytesPerPacket = 2;
    waveformat.mFramesPerPacket = 1;
    waveformat.mBytesPerFrame = 2;
    waveformat.mChannelsPerFrame = 1;
    waveformat.mBitsPerChannel = 16;
    
    err = AudioUnitGetProperty(outau, kAudioUnitProperty_StreamFormat,
        kAudioUnitScope_Output, 0, &outformat, &size);
    if (err != noErr) {
        cerr << "Error getting output audio stream format." << endl;
        CloseComponent(outau);
        return -1;
    }
    
    err = AudioConverterNew(&waveformat, &outformat, &outconv);
    if (err != noErr) {
        cerr << "Error creating audio converter." << endl;
        CloseComponent(outau);
        return -1;
    }
    
    // If the output has multiple channels (eg stereo or 5.1), map
    // our single channel onto all the output channels.
    if (outformat.mChannelsPerFrame > 1) {
        SInt32 *map = new SInt32[outformat.mChannelsPerFrame];
        for(UInt32 i = 0; i < outformat.mChannelsPerFrame; i++) {
            map[i] = 0;
        }
        
        err = AudioConverterSetProperty(outconv, kAudioConverterChannelMap,
            sizeof(SInt32) * outformat.mChannelsPerFrame, map);
        if (err != noErr) {
            cerr << "Error settomg up channel map." << endl;
            delete [] map;
            AudioConverterDispose(outconv);
            CloseComponent(outau);
            return -1;
        }
    
        delete [] map;
    }
    
    err = AudioOutputUnitStart(outau);
    if (err != noErr) {
        cerr << "Error starting audio outup." << endl;
        AudioConverterDispose(outconv);
        CloseComponent(outau);
        return -1;
    }
    
    // Poll every 50ms whether the sound has stopped playing yet.
    // Probably not the best way of doing things, but the overhead
    // should be minimal.
    size = sizeof(UInt32);
    do {
        usleep(50 * 1000);
        err = AudioUnitGetProperty(outau, kAudioOutputUnitProperty_IsRunning,
            kAudioUnitScope_Global, 0, &running, &size);
    } while (err == noErr && running);
    
    AudioConverterDispose(outconv);
    CloseComponent(outau);
    
    return 1;
}
#else

int macosx_supported = FALSE;

int play_macosx_wave(EST_Wave &inwave, EST_Option &al)
{
    (void)inwave;
    (void)al;
    cerr << "MacOS X audio support not compiled." << endl;
    return -1;
}
#endif
