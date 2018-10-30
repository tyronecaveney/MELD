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

#include "../JuceLibraryCode/JuceHeader.h"
#include "GraphEditorPanel.h"
#include "InternalFilters.h"
#include "MainHostWindow.h"

//==============================================================================
struct GraphEditorPanel::PinComponent   : public Component,
                                          public SettableTooltipClient
{
    PinComponent (GraphEditorPanel& p, AudioProcessorGraph::NodeAndChannel pinToUse, bool isIn)
        : panel (p), graph (p.graph), pin (pinToUse), isInput (isIn)
    {
        if (auto node = graph.graph.getNodeForId (pin.nodeID)) 
        {
            String tip;

            if (pin.isMIDI())
            {
                tip = isInput ? "MIDI Input"
                              : "MIDI Output";
            }
            else
            {
                auto& processor = *node->getProcessor();
                auto channel = processor.getOffsetInBusBufferForAbsoluteChannelIndex (isInput, pin.channelIndex, busIdx);

                if (auto* bus = processor.getBus (isInput, busIdx))
                    tip = bus->getName() + ": " + AudioChannelSet::getAbbreviatedChannelTypeName (bus->getCurrentLayout().getTypeOfChannel (channel));
                else
                    tip = (isInput ? "Main Input: "
                                   : "Main Output: ") + String (pin.channelIndex + 1);

            }

            setTooltip (tip);
        }

        setSize (16, 16);
    }

    void paint (Graphics& g) override
    {
        auto w = (float) getWidth();
        auto h = (float) getHeight();

        Path p;
        p.addEllipse (w * 0.25f, h * 0.25f, w * 0.5f, h * 0.5f);
        p.addRectangle (w * 0.4f, isInput ? (0.5f * h) : 0.0f, w * 0.2f, h * 0.5f);

        auto colour = (pin.isMIDI() ? Colours::red : Colours::green);

        g.setColour (colour.withRotatedHue (busIdx / 5.0f));
        g.fillPath (p);
    }

    void mouseDown (const MouseEvent& e) override
    {
        AudioProcessorGraph::NodeAndChannel dummy { 0, 0 };

        panel.beginConnectorDrag (isInput ? dummy : pin,
                                  isInput ? pin : dummy,
                                  e);
    }

    void mouseDrag (const MouseEvent& e) override
    {
        panel.dragConnector (e);
    }

    void mouseUp (const MouseEvent& e) override
    {
        panel.endDraggingConnector (e);
    }

    GraphEditorPanel& panel;
    FilterGraph& graph;
    AudioProcessorGraph::NodeAndChannel pin;
    const bool isInput;
    int busIdx = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PinComponent)
};

//==============================================================================
struct GraphEditorPanel::FilterComponent   : public Component
{
    FilterComponent (GraphEditorPanel& p, uint32 id)  : panel (p), graph (p.graph), pluginID (id)
    {
        shadow.setShadowProperties (DropShadow (Colours::black.withAlpha (0.5f), 3, { 0, 1 }));
        setComponentEffect (&shadow);

        setSize (150, 60);
    }

    FilterComponent (const FilterComponent&) = delete;
    FilterComponent& operator= (const FilterComponent&) = delete;

    void mouseDown (const MouseEvent& e) override
    {
        originalPos = localPointToGlobal (Point<int>());

        toFront (true);

        if (e.mods.isPopupMenu())
            showPopupMenu();
    }

    void mouseDrag (const MouseEvent& e) override
    {
        if (! e.mods.isPopupMenu())
        {
            auto pos = originalPos + e.getOffsetFromDragStart();

            if (getParentComponent() != nullptr)
                pos = getParentComponent()->getLocalPoint (nullptr, pos);

            pos += getLocalBounds().getCentre();

            graph.setNodePosition (pluginID,
                                   { pos.x / (double) getParentWidth(),
                                     pos.y / (double) getParentHeight() });

            panel.updateComponents();
        }
    }

    void mouseUp (const MouseEvent& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown())
        {
            graph.setChangedFlag (true);
        }
        else if (e.getNumberOfClicks() == 2)                //GRAPH DOUBLE CLICK
        {
            std::cout << pluginID << std::endl;
           if (auto f = graph.graph.getNodeForId (pluginID))
                if (auto* w = graph.getOrCreateWindowFor (f, PluginWindow::Type::normal))
                    w->toFront (true);
            
        }
    }

    bool hitTest (int x, int y) override
    {
        for (auto* child : getChildren())
            if (child->getBounds().contains (x, y))
                return true;

        return x >= 3 && x < getWidth() - 6 && y >= pinSize && y < getHeight() - pinSize;
    }

    void paint (Graphics& g) override
    {
        auto boxArea = getLocalBounds().reduced (4, pinSize);

        g.setColour (findColour (TextEditor::backgroundColourId));
        g.fillRect (boxArea.toFloat());

        g.setColour (findColour (TextEditor::textColourId));
        g.setFont (font);
        g.drawFittedText (getName(), boxArea, Justification::centred, 2);
    }
//---------------------------------------------------------------------------------------------------
    void resized() override
    {
        if (auto f = graph.graph.getNodeForId (pluginID))
        {
            if (auto* processor = f->getProcessor())
            {
                for (auto* pin : pins)
                {
                    const bool isInput = pin->isInput;
                    auto channelIndex = pin->pin.channelIndex;
                    int busIdx = 0;
                    processor->getOffsetInBusBufferForAbsoluteChannelIndex (isInput, channelIndex, busIdx);

                    const int total = isInput ? numIns : numOuts;
                    const int index = pin->pin.isMIDI() ? (total - 1) : channelIndex;

                    auto totalSpaces = static_cast<float> (total) + (static_cast<float> (jmax (0, processor->getBusCount (isInput) - 1)) * 0.5f);
                    auto indexPos = static_cast<float> (index) + (static_cast<float> (busIdx) * 0.5f);

                    pin->setBounds (proportionOfWidth ((1.0f + indexPos) / (totalSpaces + 1.0f)) - pinSize / 2,
                                    pin->isInput ? 0 : (getHeight() - pinSize),
                                    pinSize, pinSize);
                }
            }
        }
    }

    Point<float> getPinPos (int index, bool isInput) const
    {
        for (auto* pin : pins)
            if (pin->pin.channelIndex == index && isInput == pin->isInput)
                return getPosition().toFloat() + pin->getBounds().getCentre().toFloat();

        return {};
    }
//---------------------------------------------------------------------------------------------------------------------
    void update()
    {
        const AudioProcessorGraph::Node::Ptr f (graph.graph.getNodeForId (pluginID));
        jassert (f != nullptr);
        

        numIns = f->getProcessor()->getTotalNumInputChannels();
        if (f->getProcessor()->acceptsMidi())
            ++numIns;

        numOuts = f->getProcessor()->getTotalNumOutputChannels();
        if (f->getProcessor()->producesMidi())
            ++numOuts;

        int w = 100;
        int h = 60;

        w = jmax (w, (jmax (numIns, numOuts) + 1) * 20);

        const int textWidth = font.getStringWidth (f->getProcessor()->getName());
        w = jmax (w, 16 + jmin (textWidth, 300));
        if (textWidth > 300)
            h = 100;

        setSize (w, h);

        setName (f->getProcessor()->getName());

        {
            auto p = graph.getNodePosition (pluginID);
            setCentreRelative ((float) p.x, (float) p.y);
        }

        if (numIns != numInputs || numOuts != numOutputs)
        {
            numInputs = numIns;
            numOutputs = numOuts;

            pins.clear();

            for (int i = 0; i < f->getProcessor()->getTotalNumInputChannels(); ++i)
                addAndMakeVisible (pins.add (new PinComponent (panel, { pluginID, i }, true)));

            if (f->getProcessor()->acceptsMidi())
                addAndMakeVisible (pins.add (new PinComponent (panel, { pluginID, AudioProcessorGraph::midiChannelIndex }, true)));

            for (int i = 0; i < f->getProcessor()->getTotalNumOutputChannels(); ++i)
                addAndMakeVisible (pins.add (new PinComponent (panel, { pluginID, i }, false)));

            if (f->getProcessor()->producesMidi())
                addAndMakeVisible (pins.add (new PinComponent (panel, { pluginID, AudioProcessorGraph::midiChannelIndex }, false)));

            resized();
        }
    }

    AudioProcessor* getProcessor() const
    {
        if (auto node = graph.graph.getNodeForId (pluginID))
            return node->getProcessor();

        return {};
    }

    void showPopupMenu()
    {
        PopupMenu m;
        m.addItem (1, "Delete this filter");
        m.addItem (2, "Disconnect all pins");
        m.addSeparator();
        m.addItem (10, "Show plugin GUI");
        m.addItem (11, "Show all programs");
        m.addItem (12, "Show all parameters");
        m.addSeparator();
        m.addItem (20, "Configure Audio I/O");
        m.addItem (21, "Test state save/load");

        switch (m.show())
        {
            case 1:   graph.graph.removeNode (pluginID); break;
            case 2:   graph.graph.disconnectNode (pluginID); break;
            case 10:  showWindow (PluginWindow::Type::normal); break;//displays Plugin GUI
            case 11:  showWindow (PluginWindow::Type::programs); break;
            case 12:  showWindow (PluginWindow::Type::generic); break;
            case 20:  showWindow (PluginWindow::Type::audioIO); break;
            case 21:  testStateSaveLoad(); break;
            default:  break;
        }
    }

    void testStateSaveLoad()
    {
        if (auto* processor = getProcessor())
        {
            MemoryBlock state;
            processor->getStateInformation (state);
            processor->setStateInformation (state.getData(), (int) state.getSize());
        }
    }

    void showWindow (PluginWindow::Type type)
    {
        if (auto node = graph.graph.getNodeForId (pluginID))
            if (auto* w = graph.getOrCreateWindowFor (node, type))
                w->toFront (true);
    }

    GraphEditorPanel& panel;
    FilterGraph& graph;
    const AudioProcessorGraph::NodeID pluginID;
    OwnedArray<PinComponent> pins;
    int numInputs = 0, numOutputs = 0;
    int pinSize = 16;
    Point<int> originalPos;
    Font font { 13.0f, Font::bold };
    int numIns = 0, numOuts = 0;
    DropShadowEffect shadow;
};


//==============================================================================
struct GraphEditorPanel::ConnectorComponent   : public Component,
                                                public SettableTooltipClient
{
    ConnectorComponent (GraphEditorPanel& p) : panel (p), graph (p.graph)
    {
        setAlwaysOnTop (true);
    }

    void setInput (AudioProcessorGraph::NodeAndChannel newSource)
    {
        if (connection.source != newSource)
        {
            connection.source = newSource;
            update();
        }
    }

    void setOutput (AudioProcessorGraph::NodeAndChannel newDest)
    {
        if (connection.destination != newDest)
        {
            connection.destination = newDest;
            update();
        }
    }

    void dragStart (Point<float> pos)
    {
        lastInputPos = pos;
        resizeToFit();
    }

    void dragEnd (Point<float> pos)
    {
        lastOutputPos = pos;
        resizeToFit();
    }

    void update()
    {
        Point<float> p1, p2;
        getPoints (p1, p2);

        if (lastInputPos != p1 || lastOutputPos != p2)
            resizeToFit();
    }

    void resizeToFit()
    {
        Point<float> p1, p2;
        getPoints (p1, p2);

        auto newBounds = Rectangle<float> (p1, p2).expanded (4.0f).getSmallestIntegerContainer();

        if (newBounds != getBounds())
            setBounds (newBounds);
        else
            resized();

        repaint();
    }

    void getPoints (Point<float>& p1, Point<float>& p2) const
    {
        p1 = lastInputPos;
        p2 = lastOutputPos;

        if (auto* src = panel.getComponentForFilter (connection.source.nodeID))
            p1 = src->getPinPos (connection.source.channelIndex, false);

        if (auto* dest = panel.getComponentForFilter (connection.destination.nodeID))
            p2 = dest->getPinPos (connection.destination.channelIndex, true);
    }

    void paint (Graphics& g) override
    {
        if (connection.source.isMIDI() || connection.destination.isMIDI())
            g.setColour (Colours::red);
        else
            g.setColour (Colours::green);

        g.fillPath (linePath);
    }

    bool hitTest (int x, int y) override
    {
        auto pos = Point<int> (x, y).toFloat();

        if (hitPath.contains (pos))
        {
            double distanceFromStart, distanceFromEnd;
            getDistancesFromEnds (pos, distanceFromStart, distanceFromEnd);

            // avoid clicking the connector when over a pin
            return distanceFromStart > 7.0 && distanceFromEnd > 7.0;
        }

        return false;
    }

    void mouseDown (const MouseEvent&) override
    {
        dragging = false;
    }

    void mouseDrag (const MouseEvent& e) override
    {
        if (dragging)
        {
            panel.dragConnector (e);
        }
        else if (e.mouseWasDraggedSinceMouseDown())
        {
            dragging = true;

            graph.graph.removeConnection (connection);

            double distanceFromStart, distanceFromEnd;
            getDistancesFromEnds (getPosition().toFloat() + e.position, distanceFromStart, distanceFromEnd);
            const bool isNearerSource = (distanceFromStart < distanceFromEnd);

            AudioProcessorGraph::NodeAndChannel dummy { 0, 0 };

            panel.beginConnectorDrag (isNearerSource ? dummy : connection.source,
                                      isNearerSource ? connection.destination : dummy,
                                      e);
        }
    }

    void mouseUp (const MouseEvent& e) override
    {
        if (dragging)
            panel.endDraggingConnector (e);
    }

    void resized() override
    {
        Point<float> p1, p2;
        getPoints (p1, p2);

        lastInputPos = p1;
        lastOutputPos = p2;

        p1 -= getPosition().toFloat();
        p2 -= getPosition().toFloat();

        linePath.clear();
        linePath.startNewSubPath (p1);
        linePath.cubicTo (p1.x, p1.y + (p2.y - p1.y) * 0.33f,
                          p2.x, p1.y + (p2.y - p1.y) * 0.66f,
                          p2.x, p2.y);

        PathStrokeType wideStroke (8.0f);
        wideStroke.createStrokedPath (hitPath, linePath);

        PathStrokeType stroke (2.5f);
        stroke.createStrokedPath (linePath, linePath);

        auto arrowW = 5.0f;
        auto arrowL = 4.0f;

        Path arrow;
        arrow.addTriangle (-arrowL, arrowW,
                           -arrowL, -arrowW,
                           arrowL, 0.0f);

        arrow.applyTransform (AffineTransform()
                                .rotated (MathConstants<float>::halfPi - (float) atan2 (p2.x - p1.x, p2.y - p1.y))
                                .translated ((p1 + p2) * 0.5f));

        linePath.addPath (arrow);
        linePath.setUsingNonZeroWinding (true);
    }

    void getDistancesFromEnds (Point<float> p, double& distanceFromStart, double& distanceFromEnd) const
    {
        Point<float> p1, p2;
        getPoints (p1, p2);

        distanceFromStart = p1.getDistanceFrom (p);
        distanceFromEnd   = p2.getDistanceFrom (p);
    }

    GraphEditorPanel& panel;
    FilterGraph& graph;
    AudioProcessorGraph::Connection connection { { 0, 0 }, { 0, 0 } };
    Point<float> lastInputPos, lastOutputPos;
    Path linePath, hitPath;
    bool dragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ConnectorComponent)
};


////////////////////////////////////////////////////////////////
//                                                            //
//                                                            //
//                                                            //
//                                                            //
//                                                            //
//                          G U I                             //
//                                                            //
//                                                            //
//                                                            //
//                                                            //
//                                                            //
////////////////////////////////////////////////////////////////
 

GraphEditorPanel::GraphEditorPanel (FilterGraph& g)  : graph (g)
{
    graph.addChangeListener (this);
    setOpaque (true);
    
    openUp = false;
    selfieTime = false;
    
    for(int i = 0; i < 4; i++)     // array storing the 18 button screen
    {
        defaultButtons[i].addListener(this);
        defaultButtons[i].setClickingTogglesState(true);
        
       // defaultButtons[i].setImages(false, true, true, PLU, 1.0f, Colours::transparentBlack, PLH, 1.0f, Colours::transparentBlack, PLH, 1.0f, Colours::transparentBlack);

        addAndMakeVisible(&buttonLabels[i]);
        buttonLabels[i].setColour(0x1000281, juce::Colours::black);
        buttonLabels[i].setJustificationType(Justification::centred);
        buttonLabels[i].setFont(Font("Helvetica", 15, 2+1));
        
        pluginPointer[i] = nullptr;
        buttonFilled[i] = false;
    }
    
    logo.addListener(this);
    logo.setClickingTogglesState(true);
    addAndMakeVisible(&logo);
    logo.setImages(false, true, true, MeldUp, 1.0f, Colours::transparentBlack, MeldHover, 1.0f, Colours::transparentBlack, MeldUp, 1.0f, Colours::transparentBlack);
    
    addAndMakeVisible(&defaultButtons[0]);
    
    addAndMakeVisible(&maxButton);
    maxButton.addListener(this);
    maxButton.setButtonText("18");
    maxButton.setColour(0x1000100, Colours::transparentBlack);
    maxButton.setColour(0x1000101, Colours::lightgrey);
    maxButton.setClickingTogglesState(true);
    
    addAndMakeVisible(&lightMode);
    lightMode.addListener(this);
    lightMode.setButtonText("popup");
    lightMode.setColour(0x1000100, Colours::transparentBlack);
    lightMode.setColour(0x1000101, Colours::lightgrey);
    lightMode.setClickingTogglesState(true);
    light = true;
    dark = false;

    currentVST = 0;
}

GraphEditorPanel::~GraphEditorPanel()
{
    graph.removeChangeListener (this);
    draggingConnector = nullptr;
    nodes.clear();
    connectors.clear();
}

void GraphEditorPanel::paint (Graphics& g)
{
    g.fillAll (Colours::white);
    auto area = getLocalBounds();
    
    if (light == true && dark == false)
    {
        Image background = ImageCache::getFromMemory (BinaryData::bglight_png, BinaryData::bglight_pngSize);
        g.drawImageAt (background, 0, 0);
        repaint();
        for (int i = 0; i < 4; i++)
        {
            defaultButtons[i].setImages(false, true, true, PLU, 1.0f, Colours::transparentBlack, PLH, 1.0f, Colours::transparentBlack, PLD, 1.0f, Colours::transparentBlack);
            buttonLabels[i].setColour(0x1000281, juce::Colours::black);
            if (buttonFilled[i] == true)
            {
                defaultButtons[i].setImages(false, true, true, NPLU, 1.0f, Colours::transparentBlack, NPLH, 1.0f, Colours::transparentBlack, NPLD, 1.0f, Colours::transparentBlack);
            }
        }
        
    }
    
    if (dark == true && light == false)
    {
        Image background = ImageCache::getFromMemory (BinaryData::bgdark_png, BinaryData::bgdark_pngSize);
        g.drawImageAt (background, 0, 0);
        repaint();
        for (int i = 0; i < 4; i++)
        {
            defaultButtons[i].setImages(false, true, true, PDU, 1.0f, Colours::transparentBlack, PDH, 1.0f, Colours::transparentBlack, PDD, 1.0f, Colours::transparentBlack);
            buttonLabels[i].setColour(0x1000281, juce::Colours::white);
            if (buttonFilled[i] == true)
            {
                defaultButtons[i].setImages(false, true, true, NPDU, 1.0f, Colours::transparentBlack, NPDH, 1.0f, Colours::transparentBlack, NPDD, 1.0f, Colours::transparentBlack);
            }
        }
        
    }
    
    if (selfieTime == true)
    {
        g.drawImageAt(selfie, getWidth() / 3, 200);
    }
    
    FlexBox fbButtons;
    fbButtons.flexWrap = FlexBox::Wrap::wrap;
    fbButtons.justifyContent = FlexBox::JustifyContent::center;
    fbButtons.alignContent = FlexBox::AlignContent::center;

    for(int i = 0; i < 4; i++)
    {
        fbButtons.items.add((FlexItem(defaultButtons[i]).withMinWidth((9*getLocalBounds().getWidth()/48)-20.0f).withMinHeight((9*getLocalBounds().getWidth()/48)-20.0f)).withMargin(10.0f));
            // to change size of buttons, change 1/8 to a SMALLER decimal, i.e., 1/16, 1/32
    }
      fbButtons.performLayout(Rectangle<float>(getWidth()/16.0f, getHeight()/16.0f, getWidth()*7.0f/8.0f, getHeight()*7.0f/8.0f));
    
    FlexBox fbLabels;
    fbLabels.flexWrap = FlexBox::Wrap::wrap;
    fbLabels.justifyContent = FlexBox::JustifyContent::center;
    fbLabels.alignContent = FlexBox::AlignContent::center;

    for(int i = 0; i < 4; i++)
    {
        fbLabels.items.add((FlexItem(buttonLabels[i]).withMinWidth((9*getLocalBounds().getWidth()/48)-20.0f).withMinHeight((9*getLocalBounds().getWidth()/48)-20.0f)).withMargin(10.0f));
    }
    fbLabels.performLayout(Rectangle<float>(getWidth()/16.0f, getHeight()/16.0f, getWidth()*7.0f/8.0f, getHeight()*7.0f/8.0f));
}

void GraphEditorPanel::buttonClicked (Button* button) //opens VST when the button is clicked
{
    for(int i = 0; i < 4; i++)
    {
        if (button == &defaultButtons[i])
        {
            if (buttonFilled[i] == false)
            {
                PopupMenu m;
                //m.getMenu().getItem(2).setChecked(true);
                if (auto* mainWindow = findParentComponentOfClass<MainHostWindow>())
                {
                    mainWindow->addPluginsToMenu (m);
                    auto r = m.show();
                    if (auto* desc = mainWindow->getChosenType (r))
                    {
                        createNewPlugin(*desc, Point<int>(0, 0)); //creates VST at position 0,0 (Top Left)
                        buttonFilled[i] = true;
                    }
                    m.getNumItems();
                    DBG(m.getNumItems());
                }
            }
        }
    }
    
    if (button ==&defaultButtons[0])
    {
        addAndMakeVisible(defaultButtons[1]);
        buttonLabels[0].setText(vstNames[0], dontSendNotification);
        if (buttonFilled[0] == true)
        {
                if (auto f = graph.graph.getNodeForId (3))
                if (auto* w = graph.getOrCreateWindowFor (f, PluginWindow::Type::normal))
                w->toFront (true);
        }
    }

    if (button ==&defaultButtons[1])
    {
        addAndMakeVisible(defaultButtons[2]);
        buttonLabels[1].setText(vstNames[1], dontSendNotification);
        if (buttonFilled[1] == true)
        {
            if (auto f = graph.graph.getNodeForId (4))
                if (auto* w = graph.getOrCreateWindowFor (f, PluginWindow::Type::normal))
                    w->toFront (true);
        }
    }
    
    if (button ==&defaultButtons[2])
    {
        addAndMakeVisible(defaultButtons[3]);
            buttonLabels[2].setText(vstNames[2], dontSendNotification);
        if (buttonFilled[2] == true){
            if (auto f = graph.graph.getNodeForId (5))
                if (auto* w = graph.getOrCreateWindowFor (f, PluginWindow::Type::normal))
                    w->toFront (true);
        }

    }
    if (button ==&defaultButtons[3])
    {
        buttonLabels[3].setText(vstNames[3], dontSendNotification);
        if (buttonFilled[3] == true){
            if (auto f = graph.graph.getNodeForId (6))
                if (auto* w = graph.getOrCreateWindowFor (f, PluginWindow::Type::normal))
                    w->toFront (true);
        }
    }
    if (button ==&maxButton)
    {
        for(int i = 0; i < 4; i++)
        {
            addAndMakeVisible(defaultButtons[i]);
        }
    }
    if (button ==&lightMode)
    {
        openUp = !openUp;
    }
    if (button ==&logo)
    {
//        selfieTime = !selfieTime;
        String about;
        
        about << "MELD is the culmination of a three-year degree in Commercial Music Technology." << newLine << newLine << "An amalgam of ‘traditional’ interfaces & new interactive technologies; MELD harnesses the power of VST’s to give music-makers access to their favourite sounds & effects - without the need for a computer." << newLine;

        DialogWindow::LaunchOptions options;
        {
        auto* body = new Label();
        body->setText (about, dontSendNotification);
        body->setColour (Label::textColourId, Colours::black);
        body->setJustificationType(Justification::centred);
        body->setFont(Font("Helvetica", 20, 0));
        options.content.setOwned (body);
        }
        
        Rectangle<int> area (0, 0, 300, 200);

        options.content->setSize (area.getWidth(), area.getHeight());

        options.dialogTitle                   = "About";
        options.dialogBackgroundColour        = Colours::transparentBlack;
        options.escapeKeyTriggersCloseButton  = true;
        options.useNativeTitleBar             = true;
        options.resizable                     = false;

        settingsWindow = options.launchAsync();

        if (settingsWindow != nullptr)
            settingsWindow->centreWithSize (500, 300);

    }
}



void GraphEditorPanel::createNewPlugin (const PluginDescription& desc, Point<int> position)
{
    graph.addPlugin (desc, position.toDouble() / Point<double> ((double) getWidth(), (double) getHeight()));
    vstNames[currentVST] = desc.descriptiveName;
    currentVST++;
}

GraphEditorPanel::FilterComponent* GraphEditorPanel::getComponentForFilter (const uint32 filterID) const
{
    for (auto* fc : nodes)
       if (fc->pluginID == filterID)
            return fc;

    return nullptr;
}

GraphEditorPanel::ConnectorComponent* GraphEditorPanel::getComponentForConnection (const AudioProcessorGraph::Connection& conn) const
{
    for (auto* cc : connectors)
        if (cc->connection == conn)
            return cc;

    return nullptr;
}

GraphEditorPanel::PinComponent* GraphEditorPanel::findPinAt (Point<float> pos) const
{
    for (auto* fc : nodes)
    {
        // NB: A Visual Studio optimiser error means we have to put this Component* in a local
        // variable before trying to cast it, or it gets mysteriously optimised away..
        auto* comp = fc->getComponentAt (pos.toInt() - fc->getPosition());

        if (auto* pin = dynamic_cast<PinComponent*> (comp))
            return pin;
    }

    return nullptr;
}

void GraphEditorPanel::resized()
{
    updateComponents();

    maxButton.setBounds(getWidth() - getWidth() + 60, getHeight() - 100, 60, 60);
   lightMode.setBounds(getWidth() - getWidth() + 220, getHeight() - 100, 60, 60);
    logo.setBounds(1260, getHeight() - 120, 150, 150);
    
}

void GraphEditorPanel::changeListenerCallback (ChangeBroadcaster*)
{
    updateComponents();
}

void GraphEditorPanel::updateComponents()
{
    for (int i = nodes.size(); --i >= 0;)
        if (graph.graph.getNodeForId (nodes.getUnchecked(i)->pluginID) == nullptr)
            nodes.remove (i);

    for (int i = connectors.size(); --i >= 0;)
        if (! graph.graph.isConnected (connectors.getUnchecked(i)->connection))
            connectors.remove (i);

    for (auto* fc : nodes)
        fc->update();

    for (auto* cc : connectors)
        cc->update();

    for (auto* f : graph.graph.getNodes())
    {
        if (getComponentForFilter (f->nodeID) == 0)
        {
            auto* comp = nodes.add (new FilterComponent (*this, f->nodeID));
        //   addAndMakeVisible (comp); //COMMENT OUT
            comp->update();
        }
    }

    for (auto& c : graph.graph.getConnections())
    {
        if (getComponentForConnection (c) == 0)
        {
            auto* comp = connectors.add (new ConnectorComponent (*this));
            //addAndMakeVisible (comp); //COMMENT OUR

            comp->setInput (c.source);
            comp->setOutput (c.destination);
        }
    }
}

void GraphEditorPanel::beginConnectorDrag (AudioProcessorGraph::NodeAndChannel source,
                                           AudioProcessorGraph::NodeAndChannel dest,
                                           const MouseEvent& e)
{
    auto* c = dynamic_cast<ConnectorComponent*> (e.originalComponent);
    connectors.removeObject (c, false);
    draggingConnector = c;

    if (draggingConnector == nullptr)
        draggingConnector = new ConnectorComponent (*this);

    draggingConnector->setInput (source);
    draggingConnector->setOutput (dest);

    //addAndMakeVisible (draggingConnector); //COMMENT OUT
    draggingConnector->toFront (false);

    dragConnector (e);
}

void GraphEditorPanel::dragConnector (const MouseEvent& e)
{
    auto e2 = e.getEventRelativeTo (this);

    if (draggingConnector != nullptr)
    {
        draggingConnector->setTooltip ({});

        auto pos = e2.position;

        if (auto* pin = findPinAt (pos))
        {
            auto connection = draggingConnector->connection;

            if (connection.source.nodeID == 0 && ! pin->isInput)
            {
                connection.source = pin->pin;
            }
            else if (connection.destination.nodeID == 0 && pin->isInput)
            {
                connection.destination = pin->pin;
            }

            if (graph.graph.canConnect (connection))
            {
                pos = (pin->getParentComponent()->getPosition() + pin->getBounds().getCentre()).toFloat();
                draggingConnector->setTooltip (pin->getTooltip());
            }
        }

        if (draggingConnector->connection.source.nodeID == 0)
            draggingConnector->dragStart (pos);
        else
            draggingConnector->dragEnd (pos);
    }
}

void GraphEditorPanel::endDraggingConnector (const MouseEvent& e)
{
    if (draggingConnector == nullptr)
        return;

    draggingConnector->setTooltip ({});

    auto e2 = e.getEventRelativeTo (this);
    auto connection = draggingConnector->connection;

    draggingConnector = nullptr;

    if (auto* pin = findPinAt (e2.position))
    {
        if (connection.source.nodeID == 0)
        {
            if (pin->isInput)
                return;

            connection.source = pin->pin;
        }
        else
        {
            if (! pin->isInput)
                return;

            connection.destination = pin->pin;
        }

        graph.graph.addConnection (connection);
    }
}

//==============================================================================
struct GraphDocumentComponent::TooltipBar   : public Component,
                                              private Timer
{
    TooltipBar()
    {
        startTimer (100);
    }
    

    void paint (Graphics& g) override
    {
        g.setFont (Font (getHeight() * 0.7f, Font::bold));
        g.setColour (Colours::peachpuff);
        g.drawFittedText (tip, 10, 0, getWidth() - 12, getHeight(), Justification::centredLeft, 1);
    }

    void timerCallback() override
    {
        String newTip;

        if (auto* underMouse = Desktop::getInstance().getMainMouseSource().getComponentUnderMouse())
            if (auto* ttc = dynamic_cast<TooltipClient*> (underMouse))
                if (! (underMouse->isMouseButtonDown() || underMouse->isCurrentlyBlockedByAnotherModalComponent()))
                    newTip = ttc->getTooltip();

        if (newTip != tip)
        {
            tip = newTip;
            repaint();
        }
    }

    String tip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TooltipBar)
};

//==============================================================================
GraphDocumentComponent::GraphDocumentComponent (AudioPluginFormatManager& fm, AudioDeviceManager& dm)
    : graph (new FilterGraph (fm)), deviceManager (dm),
      graphPlayer (getAppProperties().getUserSettings()->getBoolValue ("doublePrecisionProcessing", false))
{
    addAndMakeVisible (graphPanel = new GraphEditorPanel (*graph));

    deviceManager.addChangeListener (graphPanel);

    graphPlayer.setProcessor (&graph->graph);

    //keyState.addListener (&graphPlayer.getMidiMessageCollector());

    //addAndMakeVisible (keyboardComp = new MidiKeyboardComponent (keyState, MidiKeyboardComponent::horizontalKeyboard));
    addAndMakeVisible (statusBar = new TooltipBar());

    deviceManager.addAudioCallback (&graphPlayer);
    deviceManager.addMidiInputCallback (String(), &graphPlayer.getMidiMessageCollector());

    graphPanel->updateComponents();
}

GraphDocumentComponent::~GraphDocumentComponent()
{
    releaseGraph();

    //keyState.removeListener (&graphPlayer.getMidiMessageCollector());
}

void GraphDocumentComponent::resized()
{
    const int keysHeight = 0;
    const int statusHeight = 20;

    graphPanel->setBounds (0, 0, getWidth(), getHeight() - keysHeight);
    statusBar->setBounds (0, getHeight() - keysHeight - statusHeight, getWidth(), statusHeight);
    //keyboardComp->setBounds (0, getHeight() - keysHeight, getWidth(), keysHeight);
}

void GraphDocumentComponent::createNewPlugin (const PluginDescription& desc, Point<int> pos)
{
    graphPanel->createNewPlugin (desc, pos);
}

void GraphDocumentComponent::unfocusKeyboardComponent()
{
    keyboardComp->unfocusAllComponents();
}

void GraphDocumentComponent::releaseGraph()
{
    deviceManager.removeAudioCallback (&graphPlayer);
    deviceManager.removeMidiInputCallback (String(), &graphPlayer.getMidiMessageCollector());

    if (graphPanel != nullptr)
    {
        deviceManager.removeChangeListener (graphPanel);
        graphPanel = nullptr;
    }

   // keyboardComp = nullptr;
    statusBar = nullptr;

    graphPlayer.setProcessor (nullptr);
    graph = nullptr;
}

void GraphDocumentComponent::setDoublePrecision (bool doublePrecision)
{
    graphPlayer.setDoublePrecisionProcessing (doublePrecision);
}

bool GraphDocumentComponent::closeAnyOpenPluginWindows()
{
    return graphPanel->graph.closeAnyOpenPluginWindows();
}

