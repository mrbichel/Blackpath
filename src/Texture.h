//
//  Texture.h
//  Blackpath
//
//  Created by Johan Bichel Lindegaard on 15/02/15.
//
//

#pragma once
#include "ofMain.h"
#include "ofxThreadedImageLoader.h"
#include "ofxSyphon.h"

class BaseImage : public ofImage {
public:
    
    BaseImage() {
        //loadImage("defaultImg.png");
    }
    
    void update() {
        if(bChanged) {
            getThumb();
        }
    }
    
    // todo: reset thumb on update
    ofImage * getThumb(int _width=50, int _height=50) {
        if(isAllocated() && isUsingTexture())  {
            if(!bChanged && thumb.isAllocated() && _width == thumb.getWidth() && _height == thumb.getHeight()) return &thumb;
            
            thumb.setFromPixels(getPixels());
            // todo: maintain aspect ratio with cropping
            // todo: cache thumbs in different sizes
            // this resize is resource hunry - optimize it maybe make it threaded
            thumb.resize(_width, _height);
            
            bChanged = false;
        }
        
        return &thumb;
    };
    
    void load(ofFile _file, ofxThreadedImageLoader * threadLoader) {
        file = _file;
        
        threadLoader->loadFromDisk(*(this), file.getAbsolutePath());
        
        armed = true;
        bChanged = true;
    }
    
    bool armed = false;
    
    void setChanged() {
        bChanged = true;
    };
    
    ofFile file;
    
private:
    ofImage thumb;
    bool bChanged;
};


class Texture : public BaseImage {
public:
    
};

class SyphonTexture {
public:
    // get Thumb
    // name - title
    ofxSyphonClient * client;
    
    SyphonTexture() {
        thumbFbo.allocate(50, 50);
        client = NULL;
        armed = false;
    };
    
    void arm(ofxSyphonClient * _client) {
        client = _client;
        //_client->setup();
        //fbo.allocate(_client->getWidth(), _client->getHeight());
        armed = true;
        bChanged = true;
    }
    
    void update() {
        if(bChanged) {
            getThumb();
        }
    };
    
    void drawThumb(int _width=50, int _height=50) {
        if(client && client->isSetup()) {
            client->draw(0,0,50,50);
        } else {
            //ofBackground(255, 0, 0);
        }
    }
    
    ofImage * getThumb(int _width=50, int _height=50) {
        
        if(!bChanged && _width == thumb.getWidth() && _height == thumb.getHeight()) return &thumb;
        
        thumbFbo.begin();
        if(client && client->isSetup() && client->getTexture().isAllocated()) {
            //todo - this is not working
            //client->draw(0,0);
            ofBackground(0, 255, 0);
        } else {
            ofBackground(255, 0, 0);
        }
        ofSetColor(255,255,255);
        thumbFbo.end();
        
        ofPixels pix;
        thumbFbo.readToPixels(pix);
        thumb.setFromPixels(pix);
        // todo: maintain aspect ratio with cropping
        thumb.resize(_width, _height);
        bChanged = false;
        
        return &thumb;
    };
    
    bool armed = false;
    //ofFbo fbo;
    
private:
    string appName;
    string serverName;
    ofImage thumb;
    bool bChanged;
    ofFbo thumbFbo;
    //bool active;
};
