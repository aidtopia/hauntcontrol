# Serial MP3/WAV Players

Adrian McCarthy
DRAFT 2019-05-11

## Introduction

NOTE:  Some of these might be YX5200 rather than YX5300.

The YX5200 and YX5300 are MP3 and WAV decoder chips that can be controlled and queried using a serial protocol.  These chips are the heart of inexpensive audio modules such as the DFPlayer Mini, the Catalex serial MP3 board, and clones.

The datasheets for these chips are in Chinese, and the translations to English are incomplete, confusing, ambiguous, and sometimes wrong.  The intent of this document is to be a more complete and accurate explanation of how to interface to modules based on this chip.  The information here is based on the commonly found English translations of the datasheet, experiments with DFPlayer Mini and Catalex MP3 boards, and nuggets of information contained in the open source demos and libraries others have written to interface microcontrollers to these devices.  See the References section.

The devices I've tested may be clones.  I suspect there may be multiple versions of the chips, with slightly different sets of bugs.

### Audio Modules

* DFPlayer Mini

* Catalex Serial MP3 Player

* MP3-TF-16P or FN-M16P

The BY8001-16P is a different beast, perhaps based on an older version of these chips.  Worth supporting?  The chip has a serial mode controlled by the absence or presence of three resistors.

In all cases, there seem to be clones and perhaps counterfeits.  At full retail, these can go for as much as $10, but if you shop around, you can often get them for $2 or less, especially if you're willing to buy in bulk or wait for slow shipments from China.

## Theory of Use

Most of the datasheets provide a reference of the messages with little information about how to use them together.  We'll start with a practical exploration of how to connect and use the module, and save the message reference for later.

### Electrical Connections

#### Catalex

#### DFPlayer Mini

##### Power

You can power the DFPlayer Mini with a maximum of 5 volts DC.  The datasheet says the typical supply is 4.2 V.  You can insert a diode between a 5-volt supply and the VCC of the DFPlayer Mini to drop the voltage to approximately 4.3 V, which may improve the lifespan and reliability of the device.

Remember to tie the grounds between the DFPlayer Mini and the microcontroller.

DFPlayer Mini actually runs at 3.3 V.  An onboard regulator adjusts the supply voltage as needed.

##### Level Shifting for Serial Communication

The DFPlayer Mini's serial lines use and expect only 3.3 volts.

When using a 3.3-volt microcontroller, no special considerations are necessary.

When using a 5-volt microcontroller, however, you must use level shifting between the microcontroller's TX pin to DFPlayer Mini's RX pin.  If you don't, the TX pin may provide more current than the RX pin is designed to handle, which could damage the device.  Here are three ways to handle this:

* Limit the current with a 1 k&ohm; resistor in series.  Some of the datasheets endorse this approach.

* Use a voltage divider to bring the 5-volt output from the microcontroller down to the 3.3-volt level.

* Use an opto-isolator to relay the 5-volt signals at the 3.3-volt level.  The hobby electronics suppliers have a variety of small modules called level shifters for doing exactly this.

In the opposite direction, from the chip's TX to the microcontroller's RX, a direct connection is fine.  A typical 5-volt microcontroller uses a threshold of 3.0 V to determine whether a digital input is high.  Since 3.3 V is higher than 3.0 V, the microcontroller shouldn't have a problem.

##### DAC outputs

The DFPlayer Mini provides access to the outputs of the DACs (digital-to-analog converters) for the left and right channels.

The DAC signals are DC biased about 200 mV.

For an audio file at maximum volume, the DAC signals range approximately from -600 mV to +600 mV (relative to the bias level).  That's a peak-to-peak range of 1.2 V and an RMS voltage of 425 mV.  These signals can power small earphones directly or be routed into an amplifier.

When a file starts playing, the amplitude ramps up to the nominal value in about 50 ms.

Do not send a DAC output directly to a GPIO pin.  Most microcontrollers require inputs to be between 0 V and the chip's operating voltage (e.g. 5 V).  The DAC outputs swing negative, which could damage the microcontroller.  You could bias the signal to ensure it's always positive and also clip it to the allowed range to make it safe for interfacing to a GPIO pin.

##### SPK outputs

There are two SPK outputs on the DFPlayer Mini.  They to _not_ correspond to the left and right audio channels.  The left and right channels are mixed down to a single channel which amplified by a low-power amplifier built into the module.  The SPK+ and SPK- signals together comprise a balanced output.

You can connect a small 8-ohm 3-watt speaker directly to the SPK pins.

The SPK outputs range approximately from 0 V to 4 V.  When no sound it being played, they float at roughly the middle of their range.  When sound is being played, the SPK- output is the inverse of the SPK+.

##### Optional USB Socket

### Source Devices

Other documentation often refers to the SD card source as TF, which I think stands for "True Flash."

### Serial Protocol

The chip communicates at 9600 baud only.  If using an Arduino, you can use a hardware or software serial port.  If using the DFPlayer Mini, be sure to make the necessary level shift between the Arduino's TX output and the board's RX input.

#### Message Format

| Purpose | Value | Notes |
| :--- | :---: | :--- |
| Start Byte | 0x7F | Indicates the beginning of a message. |
| Version | 0xFF | Significance unknown. |
| Length | 0x06 _typically_ | Number of message bytes, excluding Start Byte, Checksum, and End Byte.  The length is 0x06 for most messages, but it can be higher for Command 0x21, which can set a playlist. |
| Msg ID | _varies_ | The following tables describe the meaning of the various message ID values. |
| Feedback | 0x00 _or_ 0x01 | Use 0x01 to request a response to this message. |
| Param Hi | _varies_ | High byte of the 16-bit parameter value used by some commands.  Commands that don't use the parameter should pass 0. |
| Param Lo | _varies_ | Low byte of the 16-bit parameter value used by some commands.  Commands that don't use the parameter should pass 0. |
| Chksum Hi | _varies_ | High byte of the _optional_ 16-bit checksum. |
| Chksum Lo | _varies_ | Low byte of the _optional_ 16-bit checksum. |
| End Byte | 0xEF | Indicates the end of a message.  Note that it's almost impossible for Chksum Hi to be 0xEF, which is how a receiver can tell whether the message includes a checksum. |

Note that, instead of Param Hi and Param Lo, the Playlist command includes a list of one or more parameter bytes.  This is the one known case where Length will be something other than 0x06.

The checksum is the two's complement of the 16-bit sum of the data bytes excluding the start byte, the end byte, and the checksum itself.  Since most (all?) microcontrollers use two's complement internally code samples usually compute this as:

```C++
const std::uint16_t sum = version + length + command + feedback + param_hi + param_lo;
return 0 - sum;
```

Depending on the compiler and the warning level, the above code can generate warnings about implicit conversions.  To avoid such warnings, I prefer to explicitly compute two's complement like this:

```C++
const std::uint16_t sum = version + length + command + feedback + param_hi + param_lo;
return ~sum + 1;
```

#### ACKs and Errors

#### Initialization

Upon power-up or sending the Reset (0x0C) command, the module initializes itself.  Depending on the number of source devices available and the number of folders and files on those devices, this can take anywhere from a couple hundred milliseconds to a few seconds.

If the modules resets successfully, it sends an Initialization Complete message (0x3F).  The low byte of the parameter is a bitmask indicating which source devices are available.

> In practice, if you have both an SD card and a USB drive inserted, the initialization complete message only sets the bit for the USB device.  So if you get an initialization complete mentioning only the USB, you can check if there's also an SD card by querying the number of files on the SD card.  If that returns a number other than 0, the SD card is also available.

If there are no source devices installed during a reset, initialization ends with an Error (0x40) and a code of 1, which means the module is unavailable or that there are no devices online.

In either case, you can track changes to the availability of source devices by watch for asynchronous Device Inserted (0x3A) and Device Removed (0x3B) messages.  Those seem to be quite reliable.

Once initialization is complete, the module will probably default to selecting one of the source devices as the "current" device.  If the USB is among the devices online, it will be selected.  If only the SD card is online, it will be selected.  Nonetheless, it is highly recommended to explicitly select the device you want with the Select Source (0x09) message.

> Unfortunately, the Select Source message doesn't seem to do any error validation.  If you attempt to select a source that's not online, the selection doesn't actually change and there's no error message.  If you attempt to select an undefined device, you also don't get an error message.

> In theory, you could check which device is selected by sending the Query Status (0x42) command, but that doesn't seem to be reliable.  Except when the module is asleep, it always reports that the SD Card is selected.

Therefore the recommended procedure is:

1.  Reset the module.
2.  If you get an error code of 1, prompt the user to insert a source device and watch for the Device Inserted (0x3A) message.
3.  If you get an Initialization Complete (3F) message, verify which other devices are online by checking for non-zero file counts.
4.  Explicitly select the source device you want to use.
5.  Continue to watch for Device Inserted (0x3A) and Device Removed (0x3B) messages.

#### Module Parameters

##### Volume

DFPlayer Mini seems to be clamped to 30.

Upon initialization, Catalex reports a volume of 0, but it seems to default to the maximum setting.  Set volume commands work, and queries sent after setting the volume returns the last volume set.

Instead of clamping to 30, Catalex seems to use the lower 5 bits of the byte.  For example, if you set volume to 40, Catalex will confirm that it's set to 40, but the actual level is lower.  It's possible Catalex's top volume is 31 rather than 30.

##### Equalizer Setting

The devices have a pre-set selection of equalizer settings:

* Normal
* Pop
* Rock
* Jazz
* Classical
* Bass

On the Catalex, the equalizer selection can be changed during playback.

Changing the selection on the DFPlayer Mini, however, while a file is playing, interrupts the playback.  To safely change the EQ selection on the fly: pause, set the EQ, and resume playback.

#### Playing by Index

#### Playing by Folder and Track

#### Playing from the "MP3" Folder

#### Inserting an "Advertisement"

#### Stopping, Pausing, etc.

#### Reducing Power Consumption

When playing music at a typical volume through the amp to a 3 W, 8 Ohm speaker, the current ranges between 600 and 700 mA.  This is strange because I didn't think the power source could provide that much current.  Playing to the headphone jack draws about 27 mA.

| Mode | DFPlayer Mini | Catalex |
| :--- | ---: | ---: |
| Idle with DAC disabled | 16-17 mA | 10.5 mA |
| Idle | 17-20 mA | 13.5 mA |
| Sleeping | 20 mA | 13.5 mA |
| Playing with DAC disabled | 24 mA | 10 mA |
| Playing with DAC enabled | 27 mA | 19-20 mA |
| Playing to 3 W speaker | 600-700 mA | N/A |

TODO:  Update to check with USB sources

##### Sleep and Wake

DFPlayer Mini:  Sleep (0x0A) doesn't seem to be useful.  It does indeed change the reported state of the device, but it doesn't lower the quiescent current draw (about 20 mA).  Wake (0x0B) reports an error (module sleeping!) and doesn't seem to change much.  You can explicitly set the device (0x09) after waking, which seems to work.  But it might be more robust to just Reset (0x0C) to wake from sleeping, assuming it ever makes sense to sleep.

##### Enabling and Disabling the DACs

A better way to save a few milliamps is to disable the DAC (0x1A,1), which drops the quiescent current to 16 mA.  Reenabling the DAC (0x1A,0) seems to work just fine.

## Message Reference

### Commands

| Msg ID | Value | Parameter | Notes |
| :--- | :---: | :--- | :--- |
| Play Next File | 0x01 | 0 | Plays the next file by file index.  Wraps back to the first file after the last.  Resets the playback sequence to single. |
| Play Previous File | 0x02 | 0 | Plays the previous file by file index.  Resets the playback sequence to single. |
| Play File | 0x03 | _file index_ | This is the file system number, which is not necessarily the same as the number prefixed to the file name. TBD:  Verify range (reportedly 1-3000). |
| Volume Up | 0x04 | 0 | Increases one level unless the volume is already at level 30. |
| Volume Down | 0x05 | 0 | Decreases one level unless the volume is already at level 0. |
| Set Volume | 0x06 | DFPlayer Mini: 0-30<br>Catalex: 0-31 | Sets the volume to the selected value.  If the value is greater than 30, volume will be set to 30.  DFPlayer Mini defaults to 25.  Flyron claims default is 30. |
| Select EQ | 0x07 | 0: Normal<br>1: Pop<br>2: Rock<br>3: Jazz<br>4: Classical<br>5: Bass | On DFPlayer Mini, changing the EQ interrupts the current track. On Catalex, it doesn't. |
| Loop File | 0x08 | High: 0<br>Low: _file index_ | This command loops file index.  High byte must be zero to distinguish this from Loop Flash Track. |
| Loop Flash Track | 0x08 | High: _flash folder_<br>Low: _track_ | The high byte determines a folder in the flash memory source device and the low byte selects the track. |
| Select Source Device | 0x09 | 0: USB drive<br>1: SD card<br>2: AUX<br>3: Sleep?<br>4: Flash | When multiple sources are available, this command lets you select one as the "current" source.  AUX and FLASH are not readily available on most modules.  In theory, you can connect a USB drive. TBD:  Understand why Sleep is an option here and how it interacts with other sleep and wake commands. |
| Sleep | 0x0A | 0 | Seems pointless. |
| Wake | 0x0B | 0 | Seems buggy. |
| Reset | 0x0C | 0 | Re-initializes the module with defaults.  Initialization is not instantaneous, and it typically causes the module to send a bitmask of which sources are available. |
| Resume<br>("unpause")| 0x0D | 0 | Resume playing (typically sent when playback is paused). Can also be used 100 ms after a track finishes playing in order to play the next track. |
| Pause | 0x0E | 0 | |
| Play From Folder | 0x0F | High: _folder number_<br>Low: _track number_ | Unlike Play File Number, this selects the folder by converting the high byte of the parameter into a two-decimal-digit string.  For example, folder 0x05 means "05" and folder 0x0A means "10".  Likewise the track number is matched against the file name that begins with a three or four digit representation of the value in the low byte of the parameter.  For example, 0x1A will match a file with a name like "026 Yankee Doodle.wav". |
| Volume Adjust | 0x10 | High: _enable_<br>Low: _gain_ | This command is rejected by the modules I have.  I suspect it was to independently enable and set the volume for the speaker amplifier.  The more accurate versions of the datasheet don't list this command. |
| Loop All Files | 0x11 | 1: start looping<br>2: stop looping<br> | Plays all the files of the selected source device, repeatedly. |
| Play From "MP3" Folder | 0x12 | _track_ | Like Play From Folder, but the folder name must be "MP3" and the track number can use the full 16-bits.  For speed, it's recommended that track numbers don't exceed 3000 (0x0BB8). Doesn't work on Catalex. |
| Insert From "ADVERT" Folder | 0x13 | _track_ | Interrupts the currently playing track and plays the selected track from a folder named "ADVERT".  When the inserted track completes, the interrupted track immediately resumes from where it left off.  There is no message sent when the inserted track completes, though, on the DFPlayer Mini, you could watch for the brief blink on the BUSY line.  Documentation seems to suggest that this be used to interrupt tracks from the MP3 folder, but it works regardless of where the original track is located.  For speed, it's recommended that track numbers don't exceed 3000.  If you try to insert when nothing is playing (stopped or paused), you'll get an insertion error. Doesn't work on Catalex. |
| Play From Big Folder | 0x14 | High nybble: _folder_<br>Low 12 bits: _track_ | Like Play From Folder but allows higher track numbers as long as the folder number can be represented in 4 bits. |
| Stop "ADVERT" | 0x15 | 0 | Stops play track from "ADVERT" folder and resumes the track that was interrupted when the advertisement began. |
| Stop | 0x16 | 0 | Stops all playback.  This is distinct from Pause in that you cannot Resume. Also resets the playback sequence to single. |
| Loop Folder | 0x17 | _folder number_ | Seems legit.  Weird in that it double-ACKs. |
| Random Play | 0x18 | 0 | Plays all the files on the source device in a random order. |
| Loop Current Track | 0x19 | 0: enable looping<br>1: disable looping | Send while a track is playing (not paused or stopped). |
| DAC Enable/Disable | 0x1A | 0: enable<br>1: disable | Enables or disables the DACs (digital to audio converters) which are the audio outputs from the module.  The DACs drive the headphone jack but also the small on-board amplifier for direct speaker connection.  Presumably disabling them when not necessary saves power. |
| Playlist? | 0x21 | series of byte-sized file indexes | TODO:  Learn more.  Note: Only command with a length that's not 0x06. Doesn't seem to work reliably.  Translations call this "combined play." |
| Play With Volume | 0x22 | High: _volume_<br>Low: _file index_ | Plays the specified file at the new volume. |

### ACKs and Errors

### Asynchronous Messages

### Queries

Responses to queries use the same message format as the queries themselves.  For example, querying the status by sending 0x42 results in a response that also uses 0x42, but the response will have the requested data in the parameter.  In the following table, the Query column describes the parameter for the query, and the Response column describes the interpretation of the response.

| Msg ID | Value | Query | Response | Notes |
| :--- | :---: | :--- | :--- | :--- |
| Status | 0x42 | 0 | High: _device_<br>Low: _state_ | |
| Volume | 0x43 | 0 | _volume_ | Returns the current volume level, 0-30.  (Possibly wrong on Catalex.) |
| Equalizer | 0x44 | 0 | 0: Normal<br>1: Pop<br>2: Rock<br>3: Jazz<br>4: Classical<br>5: Bass | |
| Playback Sequence | 0x45 | 0 | 0: Loop All<br>1: Loop Folder<br>2: Loop Track<br>3: Random<br>4: Single | Indicates which sequencing mode the player is currently in. (Catalex reports Loop All upon startup, but will show the correct value when selected.) |
| Firmware Version | 0x46 | 0 | _version_ | DFPlayer Mini: 8.  Catalex: no response |
| USB File Count | 0x47 | 0 | _count_ | The _count_ is the total number of MP3 and Wave files found on the USB drive, excluding other files types, like .txt. |
| SD Card File Count | 0x48 | 0 | _count_ | The _count_ is the total number of MP3 and Wave files found on the SD card, excluding other files types, like .txt. |
| Flash File Count | 0x49 | 0 | _count_ | The _count_ is the total number of MP3 and Wave files found in the internal flash memory, excluding other files types, like .txt. |
| USB Current File | 0x4B | 0 | _file index_ | Returns the index of the current file for the USB drive. |
| SD Card Current File | 0x4C | 0 | _file index_ | Returns the index of the current file for the USB drive. |
| Flash Current File | 0x4D | 0 | _file index_ | Returns the index of the current file for the USB drive. (Catalex returns 256.) |
| Folder Track Count | 0x4E | _folder_ | _count_ | Query specifies a folder number that corresponds to a folder with a two-decimal digit name like "01" or "13".  Response returns the number of audio files in that folder. |
| Folder Count | 0x4F | 0 | _count_ | Returns the number of folders under the root folder.  Includes numbered folders like "01", the "MP3" folder if there is one, and the "ADVERT" folder if there is one. |

Queries have a natural response or an error, so it's generally not necessary to set the feedback bit in query messages.  If you do set the feedback bit, you will typically receive an ACK message immediately after the response, and the ACK's parameter will repeat the parameter from the response.

## Aidtopia YX5300 Library for Arduino

## References

[Chinese datasheet](https://github.com/0xcafed00d/yx5300/blob/master/docs/YX5300-24SS%20DEBUG%20manual%20V1.0.pdf)  Includes the 0x21 playlist(?) command, which is not necessarily of length 0x06.  Also includes the mysterious 0x61 command, which may have to do with programming the internal FLASH memory.

[A better, more complete translation of the datasheet]( http://www.flyrontech.com/uploadfile/download/20184121510393726.pdf)  Best description of commands, queries, and "ADVERT" feature.

[Catalex documentation](http://geekmatic.in.ua/pdf/Catalex_MP3_board.pdf)

[Picaxe SPE035](http://www.picaxe.com/docs/spe035.pdf) (a board for interfacing to a DFPlayer Mini)
