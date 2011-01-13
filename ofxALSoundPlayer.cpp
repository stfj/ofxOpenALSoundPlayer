/***********************************************************************
 
 Copyright (C) 2011 by Zach Gage
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 
 ************************************************************************/ 

#import "ofxALSoundPlayer.h"

bool SoundEngineInitialized = false;

UInt32	numSounds;
bool	mp3Loaded;


ofMutex soundPlayerLock;
vector<ofxALSoundPlayer *> soundPlayers;

//--------------------------------------------------------------

ofxALSoundPlayer::ofxALSoundPlayer() {

	volume = 1.0f;
	pitch = 1.0f;
	pan = 1.0f;
	stopped=true;
	bPaused=false;
	
	myPrimedId=-1;
	
	myId = 0;
	bLoadedOk=false;
	bLoop = false;
	bMultiPlay=false;
	iAmAnMp3=false;
	
	numSounds++;
}

//--------------------------------------------------------------

ofxALSoundPlayer::~ofxALSoundPlayer() { 
	
	unloadSound();
	numSounds--;
	
	soundPlayerLock.lock();
	for(int i=0;i<soundPlayers.size();i++)
	{
		if(soundPlayers[i] == this)
		{
			delete soundPlayers[i];
			break;
		}
	}
	soundPlayerLock.unlock();
	
	if(numSounds==0) {
		closeSoundEngine();
		soundPlayers.clear();
	}
}

//--------------------------------------------------------------

void ofxALSoundPlayer::loadSound(string fileName, bool stream) {
	
	if(!SoundEngineInitialized) {
		ofxALSoundPlayer::initializeSoundEngine();
	}
	
	if( fileName.length()-3 == fileName.rfind("mp3") )
		iAmAnMp3=true;
	
	if(iAmAnMp3) {
		loadBackgroundMusic(fileName, false, true);
		setLoop(bLoop);
		isStreaming=true;
	}
	else {
		isStreaming=false;
		myId = 0; //assigned by AL
		
		bLoadedOk=true;
		
		if(SoundEngine_LoadEffect(ofToDataPath(fileName).c_str(), &myId) == noErr) {
			length = SoundEngine_GetEffectLength(myId);
		}
		else {
			cerr<<"faied to load sound "<<fileName<<endl;
			bLoadedOk=false;
		}
		
		soundPlayerLock.lock();
		soundPlayers.push_back(this);
		soundPlayerLock.unlock();
	}
	
}

//--------------------------------------------------------------

void ofxALSoundPlayer::unloadSound() {
	if(bLoadedOk)
	{
		bLoadedOk=false;
		if(iAmAnMp3)
			unloadAllBackgroundMusic();
		else
			SoundEngine_UnloadEffect(myId);
	}
}

//--------------------------------------------------------------

void ofxALSoundPlayer::play() {
	if(iAmAnMp3)
		SoundEngine_StartBackgroundMusic();
	else
	{
		if(myPrimedId==-1 || bMultiPlay)
			prime();
		SoundEngine_StartEffect(myPrimedId);
	}
	
	stopped = false;
}

//--------------------------------------------------------------

void ofxALSoundPlayer::stop() {

	if(iAmAnMp3)
		SoundEngine_StopBackgroundMusic(false);
	else
		SoundEngine_StopEffect(myPrimedId);
	
	stopped = true;
}

//--------------------------------------------------------------

void ofxALSoundPlayer::setVolume(float _vol) {
	volume = _vol;
	
	if(iAmAnMp3)
		SoundEngine_SetBackgroundMusicVolume(volume);
	else
		SoundEngine_SetEffectLevel(myPrimedId, (Float32)volume);
}

//--------------------------------------------------------------

void ofxALSoundPlayer::setPan(float _pan) {
	if(iAmAnMp3)
		cerr<<"error, cannot set pan on mp3s in openAL"<<endl;
	else {
		pan = _pan*2 - 1.0f;
		setLocation(pan, 0, 0);
	}
}

//--------------------------------------------------------------

void ofxALSoundPlayer::setPitch(float _pitch) {
	if(iAmAnMp3)
		cerr<<"error, cannot set pitch on mp3s in openAL"<<endl;
	else {
		pitch = _pitch;
		SoundEngine_SetEffectPitch(myPrimedId, (Float32)pitch);
	}
}

//--------------------------------------------------------------

void ofxALSoundPlayer::setPaused(bool bP) {
	if(iAmAnMp3)
		cerr<<"error, cannot set pause on mp3s in openAL"<<endl; // TODO
	else {
		bool isPlaying = getIsPlaying();
		bPaused = bP;
		
		if(bPaused && isPlaying)
			SoundEngine_PauseEffect(myPrimedId);
		else if(!bPaused && !isPlaying)
			play();
	}
}

//--------------------------------------------------------------

void ofxALSoundPlayer::setLoop(bool bLp) {
	bLoop = bLp;
	
	if(iAmAnMp3)
		SoundEngine_SetBackgroundMusicLooping(bLoop);
	else
		SoundEngine_SetLooping(bLoop, myPrimedId);
}

//--------------------------------------------------------------

void ofxALSoundPlayer::setMultiPlay(bool bMp) { 
	if(iAmAnMp3)
		cerr<<"error, cannot set multiplay on mp3s in openAL"<<endl;
	else
		bMultiPlay = bMp;
}

//--------------------------------------------------------------

void ofxALSoundPlayer::setPosition(float pct) {
	if(iAmAnMp3)
		cerr<<"error, cannot set position on mp3s in openAL"<<endl;
	else
		SoundEngine_SetEffectPosition(myPrimedId, pct*length);
}

//--------------------------------------------------------------

float ofxALSoundPlayer::getPosition() {
	if(iAmAnMp3)
	{
		cerr<<"error, cannot get position on mp3s in openAL"<<endl;
	}
	else
		return SoundEngine_GetEffectPosition(myPrimedId);
	
	return 0;
}

//--------------------------------------------------------------

bool ofxALSoundPlayer::getIsPlaying() {
	if(iAmAnMp3)
		stopped = SoundEngine_getBackgroundMusicStopped();
	return !stopped || !bPaused;
}

//--------------------------------------------------------------

float ofxALSoundPlayer::getPitch() {
	return pitch;
}

//--------------------------------------------------------------

float ofxALSoundPlayer::getPan() {
	return pan;
}

//--------------------------------------------------------------


//static calls ---------------------------------------------------------------------------------------------------------------

void ofxALSoundPlayer::initializeSoundEngine() {
	if(!SoundEngineInitialized){
		
		OSStatus err = SoundEngine_Initialize(44100);
				
		if(err)
			cerr<<"ERROR failed to initialize soundEngine."<<endl;
		else {
			numSounds=1;
			mp3Loaded=false;
			
			SoundEngineInitialized = true;
			OSStatus err = SoundEngine_SetListenerPosition(0.0f, 0.0f, 0.0f);
			if(err)
				cerr<<"ERROR failed to set listener position in init..\n (if you are running in the simulator, this is normal, sounds won't work.)"<<endl;
		}
	}
}

//--------------------------------------------------------------

void ofxALSoundPlayer::closeSoundEngine(){
	if(SoundEngineInitialized){
		
		SoundEngine_Teardown();

		SoundEngineInitialized = false;
	}
}

//--------------------------------------------------------------

void ofxALSoundStopAll(){
	soundPlayerLock.lock();
	for(int i=0;i<soundPlayers.size();i++)
			soundPlayers[i]->stop();
	soundPlayerLock.unlock();
}

//--------------------------------------------------------------

float * ofxALSoundGetSpectrum(){
	cerr<<"unfortunately there is no way to get the sound spectrum out of openAL"<<endl;
	return NULL;
}

//--------------------------------------------------------------

void ofxALSoundSetVolume(float vol){
	if(!SoundEngineInitialized)
		ofxALSoundPlayer::initializeSoundEngine();
	
	SoundEngine_SetMasterVolume((Float32)vol);
}

//--------------------------------------------------------------



// internal ------------------------------------------------------------------------------------------------------------------

bool ofxALSoundPlayer::update() {

	for(int i=retainedBuffers.size()-1;i>=0;i--) {
		if(SoundEngine_Update(retainedBuffers[i]->primedID, retainedBuffers[i]->buffer)) {
			delete retainedBuffers[i];
			retainedBuffers.erase(retainedBuffers.begin()+i);
			stopped=true;
			if(retainedBuffers.size()==0)
				myPrimedId=-1;
			
			return true;
		}
	}
	
	return false;
}

//--------------------------------------------------------------

bool ofxALSoundPlayer::prime() {

	soundPlayerLock.lock();
	for(int i=0;i<soundPlayers.size();i++)
		soundPlayers[i]->update();
	soundPlayerLock.unlock();
	
	multiPlaySource * m;
	m = new multiPlaySource();
	ALuint newPrimedId;
	m->buffer = SoundEngine_PrimeEffect(myId, &newPrimedId);

	if(m->buffer != -1) {
		m->primedID = newPrimedId;
		myPrimedId = newPrimedId;
		retainedBuffers.push_back(m);
		updateInternalsForNewPrime();
		return true;
	}
	else
		delete m;
	
	return false;
}

//--------------------------------------------------------------

void ofxALSoundPlayer::loadBackgroundMusic(string fileName, bool queue, bool loadAtOnce) {
	myId = 0;
	
	if(!mp3Loaded) {
		if(SoundEngine_LoadBackgroundMusicTrack(ofToDataPath(fileName).c_str(), queue, loadAtOnce) == noErr )
		{
			length = SoundEngine_getBackgroundMusicLength();
			bLoadedOk=true;
			mp3Loaded=true;
			
			soundPlayerLock.lock();
				soundPlayers.push_back(this);
			soundPlayerLock.unlock();
		}
		else
		{
			bLoadedOk=false;
			cerr<<"faied to load sound "<<fileName<<endl;
		}
	}
	else {
		cerr<<"more than one mp3 cannot be loaded at the same time"<<endl;
	}
}

//--------------------------------------------------------------

void ofxALSoundPlayer::updateInternalsForNewPrime() {
	setPitch(pitch);
	setLocation(location.x, location.y, location.z);
	setPan(pan);
	setVolume(volume);
	setLoop(bLoop);
}

//--------------------------------------------------------------

void ofxALSoundPlayer::unloadAllBackgroundMusic() {
	SoundEngine_UnloadBackgroundMusicTrack();
}

//--------------------------------------------------------------

void ofxALSoundPlayer::startBackgroundMusic() {
	SoundEngine_StartBackgroundMusic();
}

//--------------------------------------------------------------

void ofxALSoundPlayer::stopBackgroundMusic(bool stopNow) {
	SoundEngine_StopBackgroundMusic(!stopNow); //this is confusing but i think stopNow makes more sense than stopAtEnd in terms of how people will likely be calling this
}

//--------------------------------------------------------------

void ofxALSoundPlayer::setBackgroundMusicVolume(float bgVol) {
	SoundEngine_SetBackgroundMusicVolume((Float32)bgVol);
}

//--------------------------------------------------------------



// beyond ofSoundPlayer ------------------------------------------------------------------------------------------------------

void ofxALSoundPlayer::vibrate() { 
	SoundEngine_Vibrate();
}

//--------------------------------------------------------------

void ofxALSoundPlayer::setLocation(float x, float y, float z) { 
	if(iAmAnMp3)
		cerr<<"error, cannot set location on mp3s in openAL"<<endl;
	else
	{
		location.set(x,y,z);
		pan = (x+1.0)/2; // this is where im compensating for the scale of oF... (pan in oF is 0-1.0, in ofxAL it's -1.0 to 1.0)
		SoundEngine_SetEffectLocation(myPrimedId, x, y, z);
	}
}

//--------------------------------------------------------------



// beyond ofSoundPlayer static -----------------------------------------------------------------------------------------------

void ofxALSoundPlayer::ofxALSoundSetListenerLocation(float x, float y, float z){	
	SoundEngine_SetListenerPosition(x, y, z);
}

//--------------------------------------------------------------
void ofxALSoundPlayer::ofxALSoundSetListenerVelocity(float x, float y, float z){
	SoundEngine_SetListenerVelocity(x, y, z); //deprecated
}

//--------------------------------------------------------------

void ofxALSoundPlayer::ofxALSoundSetListenerGain(float gain) {
	SoundEngine_SetListenerGain(gain);
}

//--------------------------------------------------------------

void ofxALSoundPlayer::ofxALSoundSetReferenceDistance(float dist) {
	SoundEngine_SetReferenceDistance(dist);
}

//--------------------------------------------------------------

void ofxALSoundPlayer::ofxALSoundSetMaxDistance(float dist) {
	SoundEngine_SetMaxDistance(dist);
}

