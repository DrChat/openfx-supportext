/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of openfx-supportext <https://github.com/devernay/openfx-supportext>,
 * Copyright (C) 2013-2017 INRIA
 *
 * openfx-supportext is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * openfx-supportext is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openfx-supportext.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

/*
 * Helper functions to implement plug-ins that support kFnOfxImageEffectPlaneSuite v2
 * In order to use these functions the following condition must be met:
 *#if defined(OFX_EXTENSIONS_NUKE) && defined(OFX_EXTENSIONS_NATRON)

   if (fetchSuite(kFnOfxImageEffectPlaneSuite, 2) &&  // for clipGetImagePlane
   getImageEffectHostDescription()->supportsDynamicChoices && // for dynamic layer choices
   getImageEffectHostDescription()->isMultiPlanar) // for clipGetImagePlane
   ... this is ok...
 *#endif
 */
#include "ofxsMultiPlane.h"

#include <algorithm>


using namespace OFX;

using std::vector;
using std::string;
using std::map;

static bool gHostSupportsMultiPlaneV1 = false;
static bool gHostSupportsMultiPlaneV2 = false;
static bool gHostSupportsDynamicChoices = false;
static bool gHostIsNatron3OrGreater = false;

static const char* rgbaComps[4] = {"R", "G", "B", "A"};
static const char* rgbComps[3] = {"R", "G", "B"};
static const char* alphaComps[1] = {"A"};
static const char* motionComps[2] = {"U", "V"};
static const char* disparityComps[2] = {"X", "Y"};
static const char* xyComps[2] = {"X", "Y"};

namespace OFX {
namespace MultiPlane {


ImagePlaneDesc::ImagePlaneDesc()
: _planeID("none")
, _planeLabel("none")
, _channels()
, _channelsLabel("none")
{
}

ImagePlaneDesc::ImagePlaneDesc(const std::string& planeID,
                               const std::string& planeLabel,
                               const std::string& channelsLabel,
                               const std::vector<std::string>& channels)
: _planeID(planeID)
, _planeLabel(planeLabel)
, _channels(channels)
, _channelsLabel(channelsLabel)
{
    if (planeLabel.empty()) {
        // Plane label is the ID if empty
        _planeLabel = _planeID;
    }
    if ( channelsLabel.empty() ) {
        // Channels label is the concatenation of all channels
        for (std::size_t i = 0; i < channels.size(); ++i) {
            _channelsLabel.append(channels[i]);
        }
    }
}

ImagePlaneDesc::ImagePlaneDesc(const std::string& planeName,
                               const std::string& planeLabel,
                               const std::string& channelsLabel,
                               const char** channels,
                               int count)
: _planeID(planeName)
, _planeLabel(planeLabel)
, _channels()
, _channelsLabel(channelsLabel)
{
    _channels.resize(count);
    for (int i = 0; i < count; ++i) {
        _channels[i] = channels[i];
    }

    if (planeLabel.empty()) {
        // Plane label is the ID if empty
        _planeLabel = _planeID;
    }
    if ( channelsLabel.empty() ) {
        // Channels label is the concatenation of all channels
        for (std::size_t i = 0; i < _channels.size(); ++i) {
            _channelsLabel.append(channels[i]);
        }
    }
}

ImagePlaneDesc::ImagePlaneDesc(const ImagePlaneDesc& other)
{
    *this = other;
}

ImagePlaneDesc&
ImagePlaneDesc::operator=(const ImagePlaneDesc& other)
{
    _planeID = other._planeID;
    _planeLabel = other._planeLabel;
    _channels = other._channels;
    _channelsLabel = other._channelsLabel;
    return *this;
}

ImagePlaneDesc::~ImagePlaneDesc()
{
}

bool
ImagePlaneDesc::isColorPlane(const std::string& planeID)
{
    return planeID == kOfxMultiplaneColorPlaneID;
}

bool
ImagePlaneDesc::isColorPlane() const
{
    return ImagePlaneDesc::isColorPlane(_planeID);
}



bool
ImagePlaneDesc::operator==(const ImagePlaneDesc& other) const
{
    if ( _channels.size() != other._channels.size() ) {
        return false;
    }
    return _planeID == other._planeID;
}

bool
ImagePlaneDesc::operator<(const ImagePlaneDesc& other) const
{
    return _planeID < other._planeID;
}

int
ImagePlaneDesc::getNumComponents() const
{
    return (int)_channels.size();
}

const std::string&
ImagePlaneDesc::getPlaneID() const
{
    return _planeID;
}

const std::string&
ImagePlaneDesc::getPlaneLabel() const
{
    return _planeLabel;
}

const std::string&
ImagePlaneDesc::getChannelsLabel() const
{
    return _channelsLabel;
}

const std::vector<std::string>&
ImagePlaneDesc::getChannels() const
{
    return _channels;
}

const ImagePlaneDesc&
ImagePlaneDesc::getNoneComponents()
{
    static const ImagePlaneDesc comp;
    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getRGBAComponents()
{
    static const ImagePlaneDesc comp(kOfxMultiplaneColorPlaneID, kOfxMultiplaneColorPlaneLabel, "", rgbaComps, 4);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getRGBComponents()
{
    static const ImagePlaneDesc comp(kOfxMultiplaneColorPlaneID, kOfxMultiplaneColorPlaneLabel, "", rgbComps, 3);

    return comp;
}


const ImagePlaneDesc&
ImagePlaneDesc::getXYComponents()
{
    static const ImagePlaneDesc comp(kOfxMultiplaneColorPlaneID, kOfxMultiplaneColorPlaneLabel, "XY", xyComps, 2);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getAlphaComponents()
{
    static const ImagePlaneDesc comp(kOfxMultiplaneColorPlaneID, kOfxMultiplaneColorPlaneLabel, "Alpha", alphaComps, 1);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getBackwardMotionComponents()
{
    static const ImagePlaneDesc comp(kOfxMultiplaneBackwardMotionVectorsPlaneID, kOfxMultiplaneBackwardMotionVectorsPlaneLabel, kOfxMultiplaneMotionComponentsLabel, motionComps, 2);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getForwardMotionComponents()
{
    static const ImagePlaneDesc comp(kOfxMultiplaneForwardMotionVectorsPlaneID, kOfxMultiplaneForwardMotionVectorsPlaneLabel, kOfxMultiplaneMotionComponentsLabel, motionComps, 2);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getDisparityLeftComponents()
{
    static const ImagePlaneDesc comp(kOfxMultiplaneDisparityLeftPlaneID, kOfxMultiplaneDisparityLeftPlaneLabel, kOfxMultiplaneDisparityComponentsLabel, disparityComps, 2);

    return comp;
}

const ImagePlaneDesc&
ImagePlaneDesc::getDisparityRightComponents()
{
    static const ImagePlaneDesc comp(kOfxMultiplaneDisparityRightPlaneID, kOfxMultiplaneDisparityRightPlaneLabel, kOfxMultiplaneDisparityComponentsLabel, disparityComps, 2);

    return comp;
}


void
ImagePlaneDesc::getChannelOption(int channelIndex, std::string* optionID, std::string* optionLabel) const
{
    if (channelIndex < 0 || channelIndex >= (int)_channels.size()) {
        assert(false);
        return;
    }

    *optionLabel += _planeLabel;
    *optionID += _planeID;
    if ( !optionLabel->empty() ) {
        *optionLabel += '.';
    }
    if (!optionID->empty()) {
        *optionID += '.';
    }

    // For the option label, append the name of the channel
    *optionLabel += _channels[channelIndex];
    *optionID += _channels[channelIndex];
}

void
ImagePlaneDesc::getPlaneOption(std::string* optionID, std::string* optionLabel) const
{
    // The option ID is always the name of the layer, this ensures for the Color plane that even if the components type changes, the choice stays
    // the same in the parameter.
    *optionLabel = _planeLabel + "." + _channelsLabel;
    *optionID = _planeID;
}

const ImagePlaneDesc&
ImagePlaneDesc::mapNCompsToColorPlane(int nComps)
{
    switch (nComps) {
        case 1:
            return ImagePlaneDesc::getAlphaComponents();
        case 2:
            return ImagePlaneDesc::getXYComponents();
        case 3:
            return ImagePlaneDesc::getRGBComponents();
        case 4:
            return ImagePlaneDesc::getRGBAComponents();
        default:
            return ImagePlaneDesc::getNoneComponents();
    }
}

static ImagePlaneDesc
ofxCustomCompToNatronComp(const std::string& comp)
{
    std::string planeID, planeLabel, channelsLabel;
    std::vector<std::string> channels;
    if (!extractCustomPlane(comp, &planeID, &planeLabel, &channelsLabel, &channels)) {
        return ImagePlaneDesc::getNoneComponents();
    }

    return ImagePlaneDesc(planeID, planeLabel, channelsLabel, channels);
}

ImagePlaneDesc
ImagePlaneDesc::mapOFXPlaneStringToPlane(const std::string& ofxPlane)
{
    assert(ofxPlane != kFnOfxImagePlaneColour);
    if (ofxPlane == kFnOfxImagePlaneBackwardMotionVector) {
        return ImagePlaneDesc::getBackwardMotionComponents();
    } else if (ofxPlane == kFnOfxImagePlaneForwardMotionVector) {
        return ImagePlaneDesc::getForwardMotionComponents();
    } else if (ofxPlane == kFnOfxImagePlaneStereoDisparityLeft) {
        return ImagePlaneDesc::getDisparityLeftComponents();
    } else if (ofxPlane == kFnOfxImagePlaneStereoDisparityRight) {
        return ImagePlaneDesc::getDisparityRightComponents();
    } else {
        return ofxCustomCompToNatronComp(ofxPlane);
    }
}

void
ImagePlaneDesc::mapOFXComponentsTypeStringToPlanes(const std::string& ofxComponents, ImagePlaneDesc* plane, ImagePlaneDesc* pairedPlane)
{
    if (ofxComponents ==  kOfxImageComponentRGBA) {
        *plane = ImagePlaneDesc::getRGBAComponents();
    } else if (ofxComponents == kOfxImageComponentAlpha) {
        *plane = ImagePlaneDesc::getAlphaComponents();
    } else if (ofxComponents == kOfxImageComponentRGB) {
        *plane = ImagePlaneDesc::getRGBComponents();
    }else if (ofxComponents == kNatronOfxImageComponentXY) {
        *plane = ImagePlaneDesc::getXYComponents();
    } else if (ofxComponents == kOfxImageComponentNone) {
        *plane = ImagePlaneDesc::getNoneComponents();
    } else if (ofxComponents == kFnOfxImageComponentMotionVectors) {
        *plane = ImagePlaneDesc::getBackwardMotionComponents();
        *pairedPlane = ImagePlaneDesc::getForwardMotionComponents();
    } else if (ofxComponents == kFnOfxImageComponentStereoDisparity) {
        *plane = ImagePlaneDesc::getDisparityLeftComponents();
        *pairedPlane = ImagePlaneDesc::getDisparityRightComponents();
    } else {
        *plane = ofxCustomCompToNatronComp(ofxComponents);
    }

} // mapOFXComponentsTypeStringToPlanes


static std::string
natronCustomCompToOfxComp(const ImagePlaneDesc &comp)
{
    std::stringstream ss;
    const std::vector<std::string>& channels = comp.getChannels();
    const std::string& planeID = comp.getPlaneID();
    const std::string& planeLabel = comp.getPlaneLabel();
    const std::string& channelsLabel = comp.getChannelsLabel();
    ss << kNatronOfxImageComponentsPlaneName << planeID;
    if (!planeLabel.empty()) {
        ss << kNatronOfxImageComponentsPlaneLabel << planeLabel;
    }
    if (!channelsLabel.empty()) {
        ss << kNatronOfxImageComponentsPlaneChannelsLabel << channelsLabel;
    }
    for (std::size_t i = 0; i < channels.size(); ++i) {
        ss << kNatronOfxImageComponentsPlaneChannel << channels[i];
    }

    return ss.str();
} // natronCustomCompToOfxComp


std::string
ImagePlaneDesc::mapPlaneToOFXPlaneString(const ImagePlaneDesc& plane)
{
    if (plane.isColorPlane()) {
        return kFnOfxImagePlaneColour;
    } else if ( plane == ImagePlaneDesc::getBackwardMotionComponents() ) {
        return kFnOfxImagePlaneBackwardMotionVector;
    } else if ( plane == ImagePlaneDesc::getForwardMotionComponents()) {
        return kFnOfxImagePlaneForwardMotionVector;
    } else if ( plane == ImagePlaneDesc::getDisparityLeftComponents()) {
        return kFnOfxImagePlaneStereoDisparityLeft;
    } else if ( plane == ImagePlaneDesc::getDisparityRightComponents() ) {
        return kFnOfxImagePlaneStereoDisparityRight;
    } else {
        return natronCustomCompToOfxComp(plane);
    }

}

std::string
ImagePlaneDesc::mapPlaneToOFXComponentsTypeString(const ImagePlaneDesc& plane)
{
    if ( plane == ImagePlaneDesc::getNoneComponents() ) {
        return kOfxImageComponentNone;
    } else if ( plane == ImagePlaneDesc::getAlphaComponents() ) {
        return kOfxImageComponentAlpha;
    } else if ( plane == ImagePlaneDesc::getRGBComponents() ) {
        return kOfxImageComponentRGB;
    } else if ( plane == ImagePlaneDesc::getRGBAComponents() ) {
        return kOfxImageComponentRGBA;
    } else if ( plane == ImagePlaneDesc::getXYComponents() ) {
        return kNatronOfxImageComponentXY;
    } else if ( plane == ImagePlaneDesc::getBackwardMotionComponents() ||
               plane == ImagePlaneDesc::getForwardMotionComponents()) {
        return kFnOfxImageComponentMotionVectors;
    } else if ( plane == ImagePlaneDesc::getDisparityLeftComponents() ||
               plane == ImagePlaneDesc::getDisparityRightComponents()) {
        return kFnOfxImageComponentStereoDisparity;
    } else {
        return natronCustomCompToOfxComp(plane);
    }
}

} // namespace MultiPlane
} // namespace OFX

namespace  {

void
getHardCodedPlanes(bool onlyColorPlane, std::vector<const MultiPlane::ImagePlaneDesc*>* planesToAdd)
{
    const MultiPlane::ImagePlaneDesc& rgbaPlane = MultiPlane::ImagePlaneDesc::getRGBAComponents();
    const MultiPlane::ImagePlaneDesc& disparityLeftPlane = MultiPlane::ImagePlaneDesc::getDisparityLeftComponents();
    const MultiPlane::ImagePlaneDesc& disparityRightPlane = MultiPlane::ImagePlaneDesc::getDisparityRightComponents();
    const MultiPlane::ImagePlaneDesc& motionBwPlane = MultiPlane::ImagePlaneDesc::getBackwardMotionComponents();
    const MultiPlane::ImagePlaneDesc& motionFwPlane = MultiPlane::ImagePlaneDesc::getForwardMotionComponents();

    planesToAdd->push_back(&rgbaPlane);
    if (!onlyColorPlane) {
        planesToAdd->push_back(&disparityLeftPlane);
        planesToAdd->push_back(&disparityRightPlane);
        planesToAdd->push_back(&motionBwPlane);
        planesToAdd->push_back(&motionFwPlane);
    }

}
void
getHardCodedPlaneOptions(const vector<string>& clips,
                         bool addConstants,
                         bool onlyColorPlane,
                         vector<string>* options,
                         vector<string>* optionsLabels,
                         vector<string>* optionHints)
{


    std::vector<const MultiPlane::ImagePlaneDesc*> planesToAdd;
    getHardCodedPlanes(onlyColorPlane, &planesToAdd);

    for (std::size_t c = 0; c < clips.size(); ++c) {
        const string& clipName = clips[c];

        for (std::size_t p = 0; p < planesToAdd.size(); ++p) {
            const std::string& planeLabel = planesToAdd[p]->getPlaneLabel();

            const std::vector<std::string>& planeChannels = planesToAdd[p]->getChannels();

            for (std::size_t i = 0; i < planeChannels.size(); ++i) {
                string opt, hint;

                // Prefix the clip name if there are multiple clip channels to read from
                if (clips.size() > 1) {
                    opt.append(clipName);
                    opt.push_back('.');
                }

                // Prefix the plane name if the plane is not the color plane
                if (planesToAdd[p] != &MultiPlane::ImagePlaneDesc::getRGBAComponents()) {
                    opt.append(planeLabel);
                    opt.push_back('.');
                }

                opt.append(planeChannels[i]);


                // Make up some tooltip
                hint.append(planeChannels[i]);
                hint.append(" channel from input ");
                hint.append(clipName);

                if (options) {
                    options->push_back(opt);
                }
                if (optionsLabels) {
                    optionsLabels->push_back(opt);
                }
                if (optionHints) {
                    optionHints->push_back(hint);
                }

            }
        }

        if ( addConstants && (c == 0) ) {
            {
                string opt, hint;
                opt.append(kMultiPlaneChannelParamOption0);
                hint.append(kMultiPlaneChannelParamOption0Hint);

                if (options) {
                    options->push_back(opt);
                }
                if (optionsLabels) {
                    optionsLabels->push_back(opt);
                }
                if (optionHints) {
                    optionHints->push_back(hint);
                }
            }
            {
                string opt, hint;
                opt.append(kMultiPlaneChannelParamOption1);
                hint.append(kMultiPlaneChannelParamOption1Hint);

                if (options) {
                    options->push_back(opt);
                }
                if (optionsLabels) {
                    optionsLabels->push_back(opt);
                }
                if (optionHints) {
                    optionHints->push_back(hint);
                }
            }
        }
    }

} // getHardCodedPlanes

template <typename T>
void
addInputChannelOptionsRGBAInternal(T* param,
                                   const vector<string>& clips,
                                   bool addConstants,
                                   bool onlyColorPlane,
                                   vector<string>* optionsParam,
                                   vector<string>* optionsLabelsParam,
                                   vector<string>* optionHintsParam)
{
    vector<string> options, labels, hints;
    getHardCodedPlaneOptions(clips, addConstants, onlyColorPlane, &options, &labels, &hints);
    if (optionsParam) {
        *optionsParam = options;
    }
    if (optionsLabelsParam) {
        *optionsLabelsParam = labels;
    }
    if (optionHintsParam) {
        *optionHintsParam = hints;
    }
    if (param) {
        for (std::size_t i = 0; i < labels.size(); ++i) {
            param->appendOption(labels[i], hints[i], options[i]);
        }
    }
} // addInputChannelOptionsRGBAInternal

} // anonymous namespace

namespace OFX {
namespace MultiPlane {

namespace Factory {
void
addInputChannelOptionsRGBA(ChoiceParamDescriptor* param,
                           const vector<string>& clips,
                           bool addConstants,
                           bool onlyColorPlane)
{
    addInputChannelOptionsRGBAInternal<ChoiceParamDescriptor>(param, clips, addConstants, onlyColorPlane, 0, 0, 0);
}

void
addInputChannelOptionsRGBA(const vector<string>& clips,
                           bool addConstants,
                           bool onlyColorPlane,
                           vector<string>* options,
                           vector<string>* optionsLabels,
                           vector<string>* optionsHints)
{
    addInputChannelOptionsRGBAInternal<ChoiceParam>(0, clips, addConstants, onlyColorPlane, options, optionsLabels, optionsHints);
}
}         // factory

/**
 * @brief For each choice param, the list of clips it "depends on" (that is the clip available planes that will be visible in the choice)
 **/
struct ChoiceParamClips
{
    // The choice parameter containing the planes or channels.
    ChoiceParam* param;

    // True if the menu should contain any entry for each channel of each plane
    bool splitPlanesIntoChannels;

    // True if we should add a "None" option
    bool addNoneOption;

    bool isOutput;

    bool hideIfClipDisconnected;

    vector<Clip*> clips;
    vector<string> clipsName;

    ChoiceParamClips()
    : param(0)
    , splitPlanesIntoChannels(false)
    , addNoneOption(false)
    , isOutput(false)
    , hideIfClipDisconnected(false)
    , clips()
    , clipsName()

    {
    }
};



struct MultiPlaneEffectPrivate
{
    // Pointer to the public interface
    MultiPlaneEffect* _publicInterface;

    // A map of each dynamic choice parameters containing planes/channels
    map<string, ChoiceParamClips> params;

    // The output clip
    Clip* dstClip;

    // If true, all planes have to be processed
    BooleanParam* allPlanesCheckbox;

    // Stores for each clip its available planes
    // This is to avoid a recursion when calling getComponentsPresent
    // on the output clip.
    std::map<Clip*, std::list<ImagePlaneDesc> > perClipPlanesAvailable;

    MultiPlaneEffectPrivate(MultiPlaneEffect* publicInterface)
    : _publicInterface(publicInterface)
    , params()
    , dstClip( publicInterface->fetchClip(kOfxImageEffectOutputClipName) )
    , allPlanesCheckbox(0)
    , perClipPlanesAvailable()
    {
    }

    /**
     * @brief The instanceChanged handler for the "All Planes" checkbox if the parameter was defined with
     **/
    void handleAllPlanesCheckboxParamChanged();

    /**
     * @brief To be called in createInstance and clipChanged to refresh visibility of input channel/plane selectors.
     **/
    void refreshSelectorsVisibility();


    /**
     * @brief Rebuild all choice parameters depending on the clips planes present.
     * This function is supposed to be called in the clipChanged action on the output clip.
     **/
    void buildChannelMenus();
};

MultiPlaneEffect::MultiPlaneEffect(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , _imp( new MultiPlaneEffectPrivate(this) )
{
}

MultiPlaneEffect::~MultiPlaneEffect()
{
}

void
MultiPlaneEffect::fetchDynamicMultiplaneChoiceParameter(const string& paramName,
                                                        bool splitPlanesIntoChannelOptions,
                                                        bool canAddNoneOption,
                                                        bool isOutputPlaneChoice,
                                                        bool hideIfClipsDisconnected,
                                                        const vector<Clip*>& dependsClips)
{
    ChoiceParamClips& paramData = _imp->params[paramName];

    paramData.param = fetchChoiceParam(paramName);
    paramData.splitPlanesIntoChannels = splitPlanesIntoChannelOptions;
    paramData.addNoneOption = canAddNoneOption;
    paramData.clips = dependsClips;

    for (std::size_t i = 0; i < dependsClips.size(); ++i) {
        paramData.clipsName.push_back( dependsClips[i]->name() );
    }

    paramData.isOutput = isOutputPlaneChoice;
    paramData.hideIfClipDisconnected = hideIfClipsDisconnected;

    if (isOutputPlaneChoice && !_imp->allPlanesCheckbox && paramExists(kMultiPlaneProcessAllPlanesParam)) {
        _imp->allPlanesCheckbox = fetchBooleanParam(kMultiPlaneProcessAllPlanesParam);
    }

    if (_imp->allPlanesCheckbox) {
        bool allPlanesSelected = _imp->allPlanesCheckbox->getValue();
        paramData.param->setIsSecretAndDisabled(allPlanesSelected);
    }

}



void
MultiPlaneEffectPrivate::buildChannelMenus()
{

    // Clear the clip planes available cache
    perClipPlanesAvailable.clear();



    // If no dynamic choices support, only add built-in planes.
    if (!gHostSupportsDynamicChoices) {
        vector<const MultiPlane::ImagePlaneDesc*> planesToAdd;
        getHardCodedPlanes(!gHostSupportsMultiPlaneV1, &planesToAdd);

        for (map<string, ChoiceParamClips>::iterator it = params.begin(); it != params.end(); ++it) {
            for (std::size_t c = 0; c < it->second.clips.size(); ++c) {
                map<Clip*,  std::list<ImagePlaneDesc> >::iterator foundClip = perClipPlanesAvailable.find(it->second.clips[c]);
                if (foundClip != perClipPlanesAvailable.end()) {
                    continue;
                } else {
                    std::list<ImagePlaneDesc>& clipPlanes = perClipPlanesAvailable[it->second.clips[c]];
                    for (vector<const MultiPlane::ImagePlaneDesc*>::const_iterator it2 = planesToAdd.begin(); it2 != planesToAdd.end(); ++it2) {
                        clipPlanes.push_back(**it2);
                    }
                }
            }
        }
        return;
    }
    
    // This code requires dynamic choice parameters support.

    // For each parameter to refresh
    for (map<string, ChoiceParamClips>::iterator it = params.begin(); it != params.end(); ++it) {

        vector<string> optionIDs, optionLabels, optionHints;


        if (it->second.splitPlanesIntoChannels) {
            // Add built-in hard-coded options A.R, A.G, ... 0, 1, B.R, B.G ...
            Factory::addInputChannelOptionsRGBA(it->second.clipsName, true /*addConstants*/, true /*onlyColorPlane*/, &optionIDs, &optionLabels, &optionHints);
        } else {
            // For plane selectors, we might want a "None" option to select an input plane.
            if (it->second.addNoneOption) {
                optionIDs.push_back(kMultiPlanePlaneParamOptionNone);
                optionLabels.push_back(kMultiPlanePlaneParamOptionNoneLabel);
                optionHints.push_back("");
            }
        }

        // We don't use a map here to keep the clips in the order of what the user passed them in fetchDynamicMultiplaneChoiceParameter
        std::list<std::pair<Clip*, std::list<ImagePlaneDesc>* > > perClipPlanes;
        for (std::size_t c = 0; c < it->second.clips.size(); ++c) {

            Clip* clip = it->second.clips[c];

            // Did we fetch the clip available planes already ? This speeds it up in the case where we have multiple choice parameters
            // accessing the same clip.
            std::list<ImagePlaneDesc>* availableClipPlanes = 0;
            map<Clip*,  std::list<ImagePlaneDesc> >::iterator foundClip = perClipPlanesAvailable.find(clip);
            if (foundClip != perClipPlanesAvailable.end()) {
                availableClipPlanes = &foundClip->second;
            } else {

                availableClipPlanes = &(perClipPlanesAvailable)[clip];

                // Fetch planes presents from the clip and map them to ImagePlaneDesc
                vector<string> clipPlaneStrings;
                clip->getComponentsPresent(&clipPlaneStrings);

                for (std::size_t i = 0; i < clipPlaneStrings.size(); ++i) {
                    ImagePlaneDesc plane;
                    if (clipPlaneStrings[i] == kOfxMultiplaneColorPlaneID) {
                        plane = ImagePlaneDesc::mapNCompsToColorPlane(clip->getPixelComponentCount());
                    } else {
                        plane = ImagePlaneDesc::mapOFXPlaneStringToPlane(clipPlaneStrings[i]);
                    }
                    availableClipPlanes->push_back(plane);
                }

            }

            perClipPlanes.push_back(std::make_pair(clip, availableClipPlanes));
        } // for each clip

        for (std::list<std::pair<Clip*, std::list<ImagePlaneDesc>* > >::const_iterator it2 = perClipPlanes.begin(); it2 != perClipPlanes.end(); ++it2) {

            const std::list<ImagePlaneDesc>* planes = it2->second;

            for (std::list<ImagePlaneDesc>::const_iterator it3 = planes->begin(); it3 != planes->end(); ++it3) {
                if (it->second.splitPlanesIntoChannels) {
                    // User wants per-channel options
                    int nChannels = it3->getNumComponents();
                    for (int k = 0; k < nChannels; ++k) {
                        optionIDs.resize(optionIDs.size() + 1);
                        optionLabels.resize(optionLabels.size() + 1);
                        optionHints.push_back("");
                        it3->getChannelOption(k, &optionIDs[optionIDs.size() - 1], &optionLabels[optionLabels.size() - 1]);

                        // Prefix the clip name if there are multiple clip channels to read from
                        if (it->second.clips.size() > 1) {
                            optionIDs[optionIDs.size() - 1] = it2->first->name() + '.' + optionIDs[optionIDs.size() - 1];
                            optionLabels[optionLabels.size() - 1] = it2->first->name() + '.' + optionLabels[optionLabels.size() - 1];
                        }

                    }
                } else {
                    // User wants planes in options
                    optionIDs.resize(optionIDs.size() + 1);
                    optionLabels.resize(optionLabels.size() + 1);
                    optionHints.push_back("");
                    it3->getPlaneOption(&optionIDs[optionIDs.size() - 1], &optionLabels[optionLabels.size() - 1]);

                    // Prefix the clip name if there are multiple clip channels to read from
                    if (it->second.clips.size() > 1) {
                        optionIDs[optionIDs.size() - 1] = it2->first->name() + '.' + optionIDs[optionIDs.size() - 1];
                        optionLabels[optionLabels.size() - 1] = it2->first->name() + '.' + optionLabels[optionLabels.size() - 1];
                    }
                }
            } // for each plane

        } // for each clip planes available

        // Set the new choice menu
        it->second.param->resetOptions(optionLabels, optionHints, optionIDs);

    } // for all choice parameters
} // buildChannelMenus

void
MultiPlaneEffectPrivate::handleAllPlanesCheckboxParamChanged()
{
    bool allPlanesSelected = allPlanesCheckbox->getValue();
    for (map<string, ChoiceParamClips>::const_iterator it = params.begin(); it != params.end(); ++it) {
        it->second.param->setIsSecretAndDisabled(allPlanesSelected);
    }
}

void
MultiPlaneEffectPrivate::refreshSelectorsVisibility()
{
    for (map<string, ChoiceParamClips>::iterator it = params.begin(); it != params.end(); ++it) {
        if ( it->second.isOutput || !it->second.hideIfClipDisconnected) {
            continue;
        }
        bool hasClipVisible = false;
        for (std::size_t i = 0; i < it->second.clips.size(); ++i) {
            if (it->second.clips[i]->isConnected()) {
                hasClipVisible = true;
                break;
            }
        }
        it->second.param->setIsSecretAndDisabled(!hasClipVisible);
    }
}

void
MultiPlaneEffect::onAllParametersFetched()
{
    _imp->refreshSelectorsVisibility();
}

void
MultiPlaneEffect::changedParam(const InstanceChangedArgs & /*args*/, const std::string &paramName)
{
    if (_imp->allPlanesCheckbox && paramName == _imp->allPlanesCheckbox->getName()) {
        _imp->handleAllPlanesCheckboxParamChanged();
    }
}

void
MultiPlaneEffect::changedClip(const InstanceChangedArgs & /*args*/, const std::string &clipName)
{
    _imp->refreshSelectorsVisibility();

    if (gHostIsNatron3OrGreater && clipName == kOfxImageEffectOutputClipName) {
        _imp->buildChannelMenus();
    }
}

void
MultiPlaneEffect::getClipPreferences(ClipPreferencesSetter &/*clipPreferences*/)
{
    // Refresh the channel menus on Natron < 3, otherwise this is done in clipChanged in Natron >= 3
    if (!gHostIsNatron3OrGreater) {
        _imp->buildChannelMenus();
    }
}

static bool findBuiltInSelectedChannel(const std::string& selectedOptionID,
                                       bool compareWithID,
                                       const ChoiceParamClips& param,
                                       MultiPlaneEffect::GetPlaneNeededRetCodeEnum* retCode,
                                       OFX::Clip** clip,
                                       ImagePlaneDesc* plane,
                                       int* channelIndexInPlane)
{
    if (selectedOptionID == kMultiPlaneChannelParamOption0) {
        *retCode = MultiPlaneEffect::eGetPlaneNeededRetCodeReturnedConstant0;
        return true;
    }

    if (selectedOptionID == kMultiPlaneChannelParamOption1) {
        *retCode = MultiPlaneEffect::eGetPlaneNeededRetCodeReturnedConstant1;
        return true;
    }

    if (param.addNoneOption && selectedOptionID == kMultiPlanePlaneParamOptionNone) {
        *plane = ImagePlaneDesc::getNoneComponents();
        *retCode = MultiPlaneEffect::eGetPlaneNeededRetCodeReturnedPlane;
        return true;
    }

    // The option must have a clip name prepended if there are multiple clips, find the clip
    std::string optionWithoutClipPrefix;

    if (param.clips.size() == 1) {
        *clip = param.clips[0];
        optionWithoutClipPrefix = selectedOptionID;
    } else {
        for (std::size_t c = 0; c < param.clipsName.size(); ++c) {
            const std::string& clipName = param.clipsName[c];
            if (selectedOptionID.substr(0, clipName.size()) == clipName) {
                *clip = param.clips[c];
                optionWithoutClipPrefix = selectedOptionID.substr(clipName.size() + 1); // + 1 to skip the dot
                break;
            }
        }
    }

    if (!*clip) {
        // We did not find the corresponding clip.
        *retCode = MultiPlaneEffect::eGetPlaneNeededRetCodeFailed;
        return false;
    }


    // Find a hard-coded option

    std::vector<const MultiPlane::ImagePlaneDesc*> planesToAdd;
    getHardCodedPlanes(false /*onlyColorPlane*/, &planesToAdd);
    for (std::size_t p = 0; p < planesToAdd.size(); ++p) {

        const vector<string>& planeChannels = planesToAdd[p]->getChannels();
        for (std::size_t c = 0; c < planeChannels.size(); ++c) {
            std::string channelOptionID;
            // For the color plane, we did not add the plane label, see @getHardCodedPlaneOptions
            if (planesToAdd[p] == &MultiPlane::ImagePlaneDesc::getRGBAComponents()) {
                channelOptionID = planeChannels[c];
            } else {
                if (compareWithID) {
                    channelOptionID = planesToAdd[p]->getPlaneID() + '.' + planeChannels[c];
                } else {
                    channelOptionID = planesToAdd[p]->getPlaneLabel() + '.' + planeChannels[c];
                }
            }
            if (channelOptionID == optionWithoutClipPrefix) {
                *plane = *planesToAdd[p];
                *channelIndexInPlane = c;
                *retCode = MultiPlaneEffect::eGetPlaneNeededRetCodeReturnedChannelInPlane;
                return true;
            }
        }

    } // for each built-in plane



    return false;
} // findBuiltInSelectedChannel

MultiPlaneEffect::GetPlaneNeededRetCodeEnum
MultiPlaneEffect::getPlaneNeeded(const std::string& paramName,
                                 OFX::Clip** clip,
                                 ImagePlaneDesc* plane,
                                 int* channelIndexInPlane)
{


    map<string, ChoiceParamClips>::iterator found = _imp->params.find(paramName);
    assert( found != _imp->params.end() );
    if ( found == _imp->params.end() ) {
        return eGetPlaneNeededRetCodeFailed;
    }


    if (found->second.isOutput && _imp->allPlanesCheckbox) {
        bool processAll = _imp->allPlanesCheckbox->getValue();
        if (processAll) {
            return eGetPlaneNeededRetCodeReturnedAllPlanes;
        }
    }


    *clip = 0;

    // Get the selected option
    string selectedOptionID;

    // By default compare option IDs, except if the host does not support it.
    bool compareWithID = true;
    {
        int choice_i;
        found->second.param->getValue(choice_i);

        if ( (0 <= choice_i) && ( choice_i < found->second.param->getNOptions() ) ) {
#ifdef OFX_EXTENSIONS_NATRON
            found->second.param->getOptionName(choice_i, selectedOptionID);
            if (selectedOptionID.empty()) {
#endif
                found->second.param->getOption(choice_i, selectedOptionID);
                compareWithID = false;
#ifdef OFX_EXTENSIONS_NATRON
            }
#endif
        } else {
            return eGetPlaneNeededRetCodeFailed;
        }
        if ( selectedOptionID.empty() ) {
            return eGetPlaneNeededRetCodeFailed;
        }

    }


    // If the choice is split by channels, check for hard coded options
    if (found->second.splitPlanesIntoChannels) {
        MultiPlaneEffect::GetPlaneNeededRetCodeEnum retCode;
        if (findBuiltInSelectedChannel(selectedOptionID, compareWithID, found->second, &retCode, clip, plane, channelIndexInPlane)) {
            return retCode;
        }
    } else {
        if (found->second.addNoneOption && selectedOptionID == kMultiPlanePlaneParamOptionNone) {
            *plane = ImagePlaneDesc::getNoneComponents();
            return  MultiPlaneEffect::eGetPlaneNeededRetCodeReturnedPlane;
        }
    } // found->second.splitPlanesIntoChannels


    // This is not a hard-coded option, check for dynamic planes
    // The option must have a clip name prepended if there are multiple clips, find the clip
    std::string optionWithoutClipPrefix;
    if (found->second.clips.size() == 1) {
        *clip = found->second.clips[0];
        optionWithoutClipPrefix = selectedOptionID;
    } else {
        for (std::size_t c = 0; c < found->second.clipsName.size(); ++c) {
            const std::string& clipName = found->second.clipsName[c];
            if (selectedOptionID.substr(0, clipName.size()) == clipName) {
                *clip = found->second.clips[c];
                optionWithoutClipPrefix = selectedOptionID.substr(clipName.size() + 1); // + 1 to skip the dot
                break;
            }
        }
    }

    if (!*clip) {
        // We did not find the corresponding clip.
        return MultiPlaneEffect::eGetPlaneNeededRetCodeFailed;
    }
    std::map<Clip*, std::list<ImagePlaneDesc> >::iterator foundPlanesPresentForClip = _imp->perClipPlanesAvailable.find(*clip);
    if (foundPlanesPresentForClip == _imp->perClipPlanesAvailable.end()) {
        // No components available for this clip...
        return MultiPlaneEffect::eGetPlaneNeededRetCodeFailed;
    }

    for (std::list<ImagePlaneDesc>::const_iterator it = foundPlanesPresentForClip->second.begin(); it != foundPlanesPresentForClip->second.end(); ++it) {
        if (found->second.splitPlanesIntoChannels) {
            // User wants per-channel options
            int nChannels = it->getNumComponents();
            for (int k = 0; k < nChannels; ++k) {
                std::string optionID, optionLabel;
                it->getChannelOption(k, &optionID, &optionLabel);

                bool foundPlane;
                if (compareWithID) {
                    foundPlane = optionWithoutClipPrefix == optionID;
                } else {
                    foundPlane = optionWithoutClipPrefix == optionLabel;
                }
                if (foundPlane) {
                    *plane = *it;
                    *channelIndexInPlane = k;
                    return eGetPlaneNeededRetCodeReturnedChannelInPlane;
                }
            }
        } else {
            // User wants planes in options
            std::string optionID, optionLabel;
            it->getPlaneOption(&optionID, &optionLabel);
            bool foundPlane;
            if (compareWithID) {
                foundPlane = optionWithoutClipPrefix == optionID;
            } else {
                foundPlane = optionWithoutClipPrefix == optionLabel;
            }
            if (foundPlane) {
                *plane = *it;
                return eGetPlaneNeededRetCodeReturnedPlane;
            }
        }


    } // for each plane available on this clip


    return eGetPlaneNeededRetCodeFailed;
} // MultiPlaneEffect::getPlaneNeededForParam


static void refreshHostFlags()
{
    gHostSupportsDynamicChoices = false;
    gHostIsNatron3OrGreater = false;
    gHostSupportsMultiPlaneV1 = false;
    gHostSupportsMultiPlaneV2 = false;

#ifdef OFX_EXTENSIONS_NATRON
    if (getImageEffectHostDescription()->supportsDynamicChoices) {
        gHostSupportsDynamicChoices = true;
    }
    if (getImageEffectHostDescription()->isNatron && getImageEffectHostDescription()->versionMajor >= 3) {
        gHostIsNatron3OrGreater = true;
    }

#endif
#ifdef OFX_EXTENSIONS_NUKE
    if (getImageEffectHostDescription()->isMultiPlanar && fetchSuite(kFnOfxImageEffectPlaneSuite, 1)) {
        gHostSupportsMultiPlaneV1 = true;
    }
    if (getImageEffectHostDescription()->isMultiPlanar && gHostSupportsDynamicChoices && fetchSuite(kFnOfxImageEffectPlaneSuite, 2)) {
        gHostSupportsMultiPlaneV2 = true;
    }
#endif
}

namespace Factory {
ChoiceParamDescriptor*
describeInContextAddPlaneChoice(ImageEffectDescriptor &desc,
                                PageParamDescriptor* page,
                                const std::string& name,
                                const std::string& label,
                                const std::string& hint)
{

    refreshHostFlags();
    if (!gHostSupportsMultiPlaneV2 && !gHostSupportsMultiPlaneV1) {
        throw std::runtime_error("Hosts does not meet requirements");
    }
    ChoiceParamDescriptor *ret;
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(name);
        param->setLabel(label);
        param->setHint(hint);
#ifdef OFX_EXTENSIONS_NATRON
        param->setHostCanAddOptions(true);             //< the host can allow the user to add custom entries
#endif
        if (!gHostSupportsMultiPlaneV2) {
            // Add hard-coded planes
            const MultiPlane::ImagePlaneDesc& rgbaPlane = MultiPlane::ImagePlaneDesc::getRGBAComponents();
            const MultiPlane::ImagePlaneDesc& disparityLeftPlane = MultiPlane::ImagePlaneDesc::getDisparityLeftComponents();
            const MultiPlane::ImagePlaneDesc& disparityRightPlane = MultiPlane::ImagePlaneDesc::getDisparityRightComponents();
            const MultiPlane::ImagePlaneDesc& motionBwPlane = MultiPlane::ImagePlaneDesc::getBackwardMotionComponents();
            const MultiPlane::ImagePlaneDesc& motionFwPlane = MultiPlane::ImagePlaneDesc::getForwardMotionComponents();

            std::vector<const MultiPlane::ImagePlaneDesc*> planesToAdd;
            planesToAdd.push_back(&rgbaPlane);
            planesToAdd.push_back(&disparityLeftPlane);
            planesToAdd.push_back(&disparityRightPlane);
            planesToAdd.push_back(&motionBwPlane);
            planesToAdd.push_back(&motionFwPlane);

            for (std::size_t i = 0; i < planesToAdd.size(); ++i) {
                std::string optionID, optionLabel;
                planesToAdd[i]->getPlaneOption(&optionID, &optionLabel);
                param->appendOption(optionLabel, "", optionID);
            }

        }
        param->setDefault(0);
        param->setAnimates(false);
        desc.addClipPreferencesSlaveParam(*param);             // < the menu is built in getClipPreferences
        if (page) {
            page->addChild(*param);
        }
        ret = param;
    }

    return ret;
}

OFX::BooleanParamDescriptor*
describeInContextAddAllPlanesOutputCheckbox(OFX::ImageEffectDescriptor &desc, OFX::PageParamDescriptor* page)
{
    refreshHostFlags();
    if (!gHostSupportsMultiPlaneV2 && !gHostSupportsMultiPlaneV1) {
        throw std::runtime_error("Hosts does not meet requirements");
    }
    BooleanParamDescriptor* param = desc.defineBooleanParam(kMultiPlaneProcessAllPlanesParam);
    param->setLabel(kMultiPlaneProcessAllPlanesParamLabel);
    param->setHint(kMultiPlaneProcessAllPlanesParamHint);
    param->setAnimates(false);
    if (page) {
        page->addChild(*param);
    }
    return param;
}

ChoiceParamDescriptor*
describeInContextAddPlaneChannelChoice(ImageEffectDescriptor &desc,
                                       PageParamDescriptor* page,
                                       const vector<string>& clips,
                                       const string& name,
                                       const string& label,
                                       const string& hint)
    
{

    refreshHostFlags();
    if (!gHostSupportsMultiPlaneV2 && !gHostSupportsMultiPlaneV1) {
        throw std::runtime_error("Hosts does not meet requirements");
    }
    
    ChoiceParamDescriptor *ret;
    {
        ChoiceParamDescriptor *param = desc.defineChoiceParam(name);
        param->setLabel(label);
        param->setHint(hint);
        param->setAnimates(false);
        addInputChannelOptionsRGBA(param, clips, true /*addContants*/, gHostSupportsMultiPlaneV2 /*onlyColorPlane*/);

        if (page) {
            page->addChild(*param);
        }
        ret = param;
    }

    return ret;
}
}         // Factory
}     // namespace MultiPlane
} // namespace OFX
