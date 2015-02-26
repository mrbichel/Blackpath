//
//  Renderer.cpp
//  Blackpath
//
//  Created by Johan Bichel Lindegaard on 2/26/15.
//
//

#include "Renderer.h"

void Renderer::setupFilters() {
    camOrientationFilter    = new ofxBiquadFilter3f(OFX_BIQUAD_TYPE_LOWPASS, 0.01, 0.7, 0.0);
    effectOrientationFilter = new ofxBiquadFilter3f(OFX_BIQUAD_TYPE_LOWPASS, 0.01, 0.7, 0.0);
    effectOffsetFilter      = new ofxBiquadFilter3f(OFX_BIQUAD_TYPE_LOWPASS, 0.01, 0.7, 0.0);
    camOffsetFilter         = new ofxBiquadFilter3f(OFX_BIQUAD_TYPE_LOWPASS, 0.01, 0.7, 0.0);
}


void Renderer::setup() {
    
    landscapeTextureFader = new TextureFader();
    secondaryTextureFader = new TextureFader();
    effectTextureFader = new TextureFader();
    skyTextureFader= new TextureFader();
    
    landscapeFader = new ModelFader();
    effectModelFader = new ModelFader();
    
    modelOffset = ofVec3f(0,0,0);
    zTravel = 0;
    
    cam.setupPerspective();
    cam.setVFlip(true);
    
    cubeMap.initEmptyTextures( 512 );
    
    
    camRefPos = cam.getPosition();
    landscapeTextureFader->setup();
    effectTextureFader->setup();
    skyTextureFader->setup();
    secondaryTextureFader->setup();
    
    setupFilters();
    setOutput();
}


void Renderer::setOutput() {
    
    // todo make output settings configurable
    
    ofFbo::Settings fboSettings;
    fboSettings.height = 1920;
    fboSettings.width  = 1080;
    fboSettings.textureTarget = GL_TEXTURE_RECTANGLE_ARB;
    fboSettings.numSamples = 8;
    fboSettings.useDepth = true;
    fboSettings.internalformat = GL_RGBA;
    //fboSettings.depthStencilAsTexture = true;
    
    fbo.allocate(fboSettings);
}

void Renderer::renderSky() {
    for( int i = 0; i < 6; i++ )
    {
        cubeMap.beginDrawingInto2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i );
        
        ofClear(0,0,0);
        
        skyTextureFader->draw(cubeMap.getWidth(), cubeMap.getHeight());
        //ofLine(0,0, cubeMap.getWidth(), cubeMap.getHeight() );
        //ofLine(cubeMap.getWidth(), 0, 0, cubeMap.getHeight() );
        
        cubeMap.endDrawingInto2D();
    }
}

void Renderer::renderLandscape() {
    
    ofPushMatrix(); {
        ofTranslate(modelOffset);
        ofPushMatrix(); {
            ofPushMatrix(); {
                
                ofTranslate(fbo.getWidth()/2,fbo.getHeight()/2);
                
                landscapeTextureFader->begin();{
                    renderLandscapeVboMeshes(landscapeFader->getCurrent(), 1-landscapeFader->tween.update(), true);
                    renderLandscapeVboMeshes(landscapeFader->getNext(), landscapeFader->tween.update(), true);
                    
                } landscapeTextureFader->end();
                
                secondaryTextureFader->begin();{
                    
                    renderLandscapeVboMeshes(landscapeFader->getCurrent(), 1-landscapeFader->tween.update(), false);
                    renderLandscapeVboMeshes(landscapeFader->getNext(), landscapeFader->tween.update(), false);
                    
                } secondaryTextureFader->end();
                
                
            }ofPopMatrix();
        } ofPopMatrix();
    } ofPopMatrix();
    

}

void Renderer::update() {
    if(params->bAutoCameraRotation.get()) {
        camOrientation += (params->autoCamSpeed.get() * ofGetLastFrameTime() * 100);
    } else {
        camOrientation = params->camOrientation.get();
    }
    
    cam.setOrientation(camOrientationFilter->updateDegree(camOrientation + params->camOrientation.get()));
    
    if(params->bAutoEffectRotation.get()) {
        effectOrientation += (params->autoEffectRotSpeed.get() * ofGetLastFrameTime() * 100);
    } else {
        effectOrientation = params->effectOrientationRef.get();
    }
    
    cam.setFarClip(params->camFarClip);
    cam.setFov(params->camFov);
    cam.setNearClip(params->camNearClip);
    zTravel -= params->camSpeed * ofGetLastFrameTime() * 1000;
    
    camOffset = camOffsetFilter->update(camRefPos + params->camOffset.get() + ofVec3f(0,0,zTravel));
    
    cam.setPosition(params->camOffset.get());
    landscapeTextureFader->update();
    effectTextureFader->update();
    skyTextureFader->update();
    secondaryTextureFader->update();
    effectModelFader->update();
    landscapeFader->update();
}



void Renderer::renderLandscapeVboMeshes(Model * m, float fade, bool prim = true) {
    if(m == NULL || !m->hasMeshes()) return;
    
    ofPushMatrix();{
        // tile forward and backward to far clip
        // float zMin = m->getSceneMin().z;
        
        // as in depth
        float zMin = abs(m->getSceneMin().z - m->getSceneMax().z);
        int tileN = ceil((cam.getFarClip()*2) / zMin) + 2;
        
        ofTranslate(ofVec3f(0,0,floor(cam.getPosition().z / zMin) * zMin));
        //cout<<m->getSceneMin()<<"   "<<m->getSceneMax()<<"   "<<m->getSceneCenter()<<endl;
        
        for(int i=0; i<tileN; i++) {
            ofPushMatrix(); {
                ofTranslate(0, 0, (zMin*i) - ((tileN/2) * zMin) );
                
                ofScale(fade,fade,fade); // TODO: have an alpha transition option also
                if(prim) {
                    
                    m->drawVboMesh(0);
                    //m->vboMeshes[0].draw();
                    
                } else {
                    for(int i=1; i <m->getMeshCount(); i++) {
                        m->drawVboMesh(i);
                        //m->vboMeshes[i].draw();
                        
                    }
                }
            }ofPopMatrix();
        }
    }ofPopMatrix();
}

void Renderer::renderEffectModel(Model * m, float fade) {
    if(m == NULL || !m->hasMeshes()) return;
    
    ofPushMatrix();
    
    ofTranslate( -m->getSceneCenter()   );
    
    ofVec3f filteredRot = effectOrientationFilter->updateDegree(effectOrientation + params->effectOrientationRef.get());
    ofRotateX(filteredRot.x);
    ofRotateY(filteredRot.y);
    ofRotateZ(filteredRot.z);
    
    ofScale(params->effectScale.get(), params->effectScale.get(), params->effectScale.get());
    
    //ofDrawGrid();
    
    ofScale(fade,fade,fade);
    
    for(int i=0; i <m->getMeshCount(); i++) {
        m->drawVboMesh(i);
        
    }
    
    ofPopMatrix();
    
}


void Renderer::render() {
    
    
    ofEnableAlphaBlending();
    //ofDisableArbTex();
    
    renderSky();
    
    ofSetColor(255);
    
    fbo.begin(); {
        //ofEnableNormalizedTexCoords();
        
        ofBackground(0,0,0);
        
        //ofEnableLighting();
        ofEnableDepthTest();
        
        skyBoxCam = cam;
        skyBoxCam.setFarClip(70000);
        
        skyBoxCam.begin(); {
            ofPushMatrix(); {
                ofTranslate(cam.getPosition());
                cubeMap.bind();
                cubeMap.drawSkybox(65000);
                cubeMap.unbind();
            } ofPopMatrix();
        } skyBoxCam.end();
        
        cam.begin(); {
            
            renderLandscape();
            
            ofPushMatrix(); {
                ofPushStyle(); {
                    ofTranslate(cam.getPosition());
                    float a, x,y,z;
                    cam.getOrientationQuat().getRotate(a, x, y, z);
                    ofRotate(a, x, y, z);
                    
                    effectOffset = effectOffsetFilter->update(params->effectOffset.get());
                    ofTranslate(effectOffset);
                    
                    // where is the center?
                    //ofSetColor(255,0,0);
                    //ofDrawSphere(0, 0, 20);
                    
                    effectTextureFader->begin(); {
                        //for(int i=0; i < effectReplicator.get().x; i++) {
                        //    ofTranslate(100, 0);
                        renderEffectModel(effectModelFader->getCurrent(), 1-effectModelFader->tween.update());
                        renderEffectModel(effectModelFader->getNext(), effectModelFader->tween.update());
                        //}
                    } effectTextureFader->end();
                    
                    
                } ofPopStyle();
            } ofPopMatrix();
            
        } cam.end();
        
        ofDisableDepthTest();
        
        /*if(directSyphon > 0) {
            landscapeTextureFader.draw(fbo.getWidth(), fbo.getHeight(), directSyphon);
        }*/
        
        
    } fbo.end();

    
    
    
}