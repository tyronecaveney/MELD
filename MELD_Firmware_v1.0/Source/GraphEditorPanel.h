/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once

#include "FilterGraph.h"
#include "MainHostWindow.h"


//==============================================================================
/**
    A panel that displays and edits a FilterGraph.
*/
class GraphEditorPanel   : public Component,
                           public ChangeListener,
                           public TextButton::Listener
{
public:
    GraphEditorPanel (FilterGraph& graph);
    ~GraphEditorPanel();

    void createNewPlugin (const PluginDescription&, Point<int> position);

    void paint (Graphics&) override;
    void resized() override;
    void changeListenerCallback (ChangeBroadcaster*) override;
    void updateComponents();
    void buttonClicked(Button* button) override;
    bool buttonFilled[4];
    int currentVST;

    //Image background = ImageCache::getFromMemory(BinaryData::LOAD_PLUGIN_png, BinaryData::LOAD_PLUGIN_pngSize);

    //==============================================================================
    void beginConnectorDrag (AudioProcessorGraph::NodeAndChannel source,
                             AudioProcessorGraph::NodeAndChannel dest,
                             const MouseEvent&);
    void dragConnector (const MouseEvent&);
    void endDraggingConnector (const MouseEvent&);

    //==============================================================================
    FilterGraph& graph;
    
    // PLUS LIGHT
    Image PLU =  ImageCache::getFromMemory (BinaryData::plusup_png, BinaryData::plusup_pngSize);
    Image PLD =  ImageCache::getFromMemory (BinaryData::plusdown_png, BinaryData::plusdown_pngSize);
    Image PLH =  ImageCache::getFromMemory (BinaryData::plushover_png, BinaryData::plushover_pngSize);
    
    //PLUS DARK
    Image PDU =  ImageCache::getFromMemory (BinaryData::plusupdark_png, BinaryData::plusupdark_pngSize);
    Image PDD =  ImageCache::getFromMemory (BinaryData::plusdowndark_png, BinaryData::plusdowndark_pngSize);
    Image PDH =  ImageCache::getFromMemory (BinaryData::plushoverdark_png, BinaryData::plushoverdark_pngSize);
    
    // NOPLUS LIGHT
    Image NPLU =  ImageCache::getFromMemory (BinaryData::noplusup_png, BinaryData::noplusup_pngSize);
    Image NPLD =  ImageCache::getFromMemory (BinaryData::noplusdown_png, BinaryData::noplusdown_pngSize);
    Image NPLH =  ImageCache::getFromMemory (BinaryData::noplushover_png, BinaryData::noplushover_pngSize);
    
    // NO PLUS DARK
    Image NPDU =  ImageCache::getFromMemory (BinaryData::noplusupdark_png, BinaryData::noplusupdark_pngSize);
    Image NPDD =  ImageCache::getFromMemory (BinaryData::noplusdowndark_png, BinaryData::noplusdowndark_pngSize);
    Image NPDH =  ImageCache::getFromMemory (BinaryData::noplushoverdark_png, BinaryData::noplushoverdark_pngSize);
    
    Image MeldUp = ImageCache::getFromMemory (BinaryData::MELDup_png, BinaryData::MELDup_pngSize);
    Image MeldHover = ImageCache::getFromMemory (BinaryData::MELDhover_png, BinaryData::MELDhover_pngSize);
    
    Image selfie = ImageCache::getFromMemory (BinaryData::selfie_png, BinaryData::selfie_pngSize);
    
    bool selfieTime;
    bool openUp;

private:
    struct FilterComponent;
    struct ConnectorComponent;
    struct PinComponent;

    OwnedArray<FilterComponent> nodes;
    OwnedArray<ConnectorComponent> connectors;
    ScopedPointer<ConnectorComponent> draggingConnector;

    FilterComponent* getComponentForFilter (AudioProcessorGraph::NodeID) const;
    ConnectorComponent* getComponentForConnection (const AudioProcessorGraph::Connection&) const;
    PinComponent* findPinAt (Point<float>) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphEditorPanel)
    
    ImageButton defaultButtons[4];
    Label buttonLabels[4];
    
    TextButton maxButton;
    TextButton lightMode;
    ImageButton logo;
    
    bool light;
    bool dark;
    bool pluginOn;
    const PluginDescription *pluginPointer[4];
    String vstNames[4];
    int buttonClick;
    
    SafePointer<DocumentWindow> settingsWindow;
};


//==============================================================================
/**
    A panel that embeds a GraphEditorPanel with a midi keyboard at the bottom.

    It also manages the graph itself, and plays it.
*/
class GraphDocumentComponent  : public Component
{
public:
    GraphDocumentComponent (AudioPluginFormatManager& formatManager,
                            AudioDeviceManager& deviceManager);
    ~GraphDocumentComponent();

    //==============================================================================
    void createNewPlugin (const PluginDescription&, Point<int> position);
    void setDoublePrecision (bool doublePrecision);
    bool closeAnyOpenPluginWindows();

    //==============================================================================
    ScopedPointer<FilterGraph> graph;

    void resized();
    void unfocusKeyboardComponent();
    void releaseGraph();

    ScopedPointer<GraphEditorPanel> graphPanel;
    ScopedPointer<MidiKeyboardComponent> keyboardComp;
    
    
private:
    //==============================================================================
    AudioDeviceManager& deviceManager;
    AudioProcessorPlayer graphPlayer;
    //MidiKeyboardState keyState;
    

    struct TooltipBar;
    ScopedPointer<TooltipBar> statusBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphDocumentComponent)
};

