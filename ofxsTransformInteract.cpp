/*
   OFX TransformInteract.

   Copyright (C) 2015 INRIA

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice, this
   list of conditions and the following disclaimer in the documentation and/or
   other materials provided with the distribution.

   Neither the name of the {organization} nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   INRIA
   Domaine de Voluceau
   Rocquencourt - B.P. 105
   78153 Le Chesnay Cedex - France
 */

#include "ofxsTransformInteract.h"

#include <memory>
#include <cmath>
#include <algorithm>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "ofxsMatrix2D.h"
#include "ofxsTransform3x3.h"

#define CIRCLE_RADIUS_BASE 30.
#define CIRCLE_RADIUS_MIN 15.
#define CIRCLE_RADIUS_MAX 300.
#define POINT_SIZE 7.
#define ELLIPSE_N_POINTS 50.

using namespace OFX;


////////////////////////////////////////////////////////////////////////////////
// stuff for the interact

TransformInteract::TransformInteract(OfxInteractHandle handle, OFX::ImageEffect* effect)
    : OFX::OverlayInteract(handle)
    , _drawState(eInActive)
    , _mouseState(eReleased)
    , _modifierStateCtrl(0)
    , _modifierStateShift(0)
    , _orientation(eOrientationAllDirections)
    , _plugin(effect)
    , _lastMousePos()
    , _scaleUniformDrag(0)
    , _rotateDrag(0)
    , _skewXDrag(0)
    , _skewYDrag(0)
    , _skewOrderDrag(0)
    , _invertedDrag(0)
    , _interactiveDrag(false)
    , _translate(0)
    , _rotate(0)
    , _scale(0)
    , _scaleUniform(0)
    , _skewX(0)
    , _skewY(0)
    , _skewOrder(0)
    , _center(0)
    , _invert(0)
    , _interactive(0)
    {

        assert(_plugin);
        _lastMousePos.x = _lastMousePos.y = 0.;
        // NON-GENERIC
        _translate = _plugin->fetchDouble2DParam(kParamTransformTranslate);
        _rotate = _plugin->fetchDoubleParam(kParamTransformRotate);
        _scale = _plugin->fetchDouble2DParam(kParamTransformScale);
        _scaleUniform = _plugin->fetchBooleanParam(kParamTransformScaleUniform);
        _skewX = _plugin->fetchDoubleParam(kParamTransformSkewX);
        _skewY = _plugin->fetchDoubleParam(kParamTransformSkewY);
        _skewOrder = _plugin->fetchChoiceParam(kParamTransformSkewOrder);
        _center = _plugin->fetchDouble2DParam(kParamTransformCenter);
        _invert = _plugin->fetchBooleanParam(kParamTransform3x3Invert);
        _interactive = _plugin->fetchBooleanParam(kParamTransformInteractive);
        assert(_translate && _rotate && _scale && _scaleUniform && _skewX && _skewY && _skewOrder && _center && _interactive);
        addParamToSlaveTo(_translate);
        addParamToSlaveTo(_rotate);
        addParamToSlaveTo(_scale);
        addParamToSlaveTo(_skewX);
        addParamToSlaveTo(_skewY);
        addParamToSlaveTo(_skewOrder);
        addParamToSlaveTo(_center);
        addParamToSlaveTo(_invert);
        _centerDrag.x = _centerDrag.y = 0.;
        _translateDrag.x = _translateDrag.y = 0.;
        _scaleParamDrag.x = _scaleParamDrag.y = 0.;
    }
    
static void getTargetCenter(const OfxPointD &center, const OfxPointD &translate, OfxPointD *targetCenter)
    {
        targetCenter->x = center.x + translate.x;
        targetCenter->y = center.y + translate.y;
    }

static void getTargetRadius(const OfxPointD& scale, const OfxPointD& pixelScale, OfxPointD* targetRadius)
    {
        targetRadius->x = scale.x * CIRCLE_RADIUS_BASE;
        targetRadius->y = scale.y * CIRCLE_RADIUS_BASE;
        // don't draw too small. 15 pixels is the limit
        if (std::fabs(targetRadius->x) < CIRCLE_RADIUS_MIN && std::fabs(targetRadius->y) < CIRCLE_RADIUS_MIN) {
            targetRadius->x = targetRadius->x >= 0 ? CIRCLE_RADIUS_MIN : -CIRCLE_RADIUS_MIN;
            targetRadius->y = targetRadius->y >= 0 ? CIRCLE_RADIUS_MIN : -CIRCLE_RADIUS_MIN;
        } else if (std::fabs(targetRadius->x) > CIRCLE_RADIUS_MAX && std::fabs(targetRadius->y) > CIRCLE_RADIUS_MAX) {
            targetRadius->x = targetRadius->x >= 0 ? CIRCLE_RADIUS_MAX : -CIRCLE_RADIUS_MAX;
            targetRadius->y = targetRadius->y >= 0 ? CIRCLE_RADIUS_MAX : -CIRCLE_RADIUS_MAX;
        } else {
            if (std::fabs(targetRadius->x) < CIRCLE_RADIUS_MIN) {
                if (targetRadius->x == 0. && targetRadius->y != 0.) {
                    targetRadius->y = targetRadius->y > 0 ? CIRCLE_RADIUS_MAX : -CIRCLE_RADIUS_MAX;
                } else {
                    targetRadius->y *= std::fabs(CIRCLE_RADIUS_MIN/targetRadius->x);
                }
                targetRadius->x = targetRadius->x >= 0 ? CIRCLE_RADIUS_MIN : -CIRCLE_RADIUS_MIN;
            }
            if (std::fabs(targetRadius->x) > CIRCLE_RADIUS_MAX) {
                targetRadius->y *= std::fabs(CIRCLE_RADIUS_MAX/targetRadius->x);
                targetRadius->x = targetRadius->x > 0 ? CIRCLE_RADIUS_MAX : -CIRCLE_RADIUS_MAX;
            }
            if (std::fabs(targetRadius->y) < CIRCLE_RADIUS_MIN) {
                if (targetRadius->y == 0. && targetRadius->x != 0.) {
                    targetRadius->x = targetRadius->x > 0 ? CIRCLE_RADIUS_MAX : -CIRCLE_RADIUS_MAX;
                } else {
                    targetRadius->x *= std::fabs(CIRCLE_RADIUS_MIN/targetRadius->y);
                }
                targetRadius->y = targetRadius->y >= 0 ? CIRCLE_RADIUS_MIN : -CIRCLE_RADIUS_MIN;
            }
            if (std::fabs(targetRadius->y) > CIRCLE_RADIUS_MAX) {
                targetRadius->x *= std::fabs(CIRCLE_RADIUS_MAX/targetRadius->x);
                targetRadius->y = targetRadius->y > 0 ? CIRCLE_RADIUS_MAX : -CIRCLE_RADIUS_MAX;
            }
        }
        // the circle axes are not aligned with the images axes, so we cannot use the x and y scales separately
        double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
        targetRadius->x *= meanPixelScale;
        targetRadius->y *= meanPixelScale;
    }

    static void getTargetPoints(const OfxPointD& targetCenter,
                   const OfxPointD& targetRadius,
                   OfxPointD *left,
                   OfxPointD *bottom,
                   OfxPointD *top,
                   OfxPointD *right)
    {
        left->x = targetCenter.x - targetRadius.x ;
        left->y = targetCenter.y;
        right->x = targetCenter.x + targetRadius.x ;
        right->y = targetCenter.y;
        top->x = targetCenter.x;
        top->y = targetCenter.y + targetRadius.y ;
        bottom->x = targetCenter.x;
        bottom->y = targetCenter.y - targetRadius.y ;
    }


static void
drawSquare(const OfxRGBColourD& color,
           const OfxPointD& center,
           const OfxPointD& pixelScale,
           bool hovered,
           bool althovered,
           int l)
{
    // we are not axis-aligned
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    if (hovered) {
        if (althovered) {
            glColor3f(0.f*l, 1.f*l, 0.f*l);
        } else {
            glColor3f(1.f*l, 0.f*l, 0.f*l);
        }
    } else {
        glColor3f((float)color.r*l, (float)color.g*l, (float)color.b*l);
    }
    double halfWidth = (POINT_SIZE / 2.) * meanPixelScale;
    double halfHeight = (POINT_SIZE / 2.) * meanPixelScale;
    glPushMatrix();
    glTranslated(center.x, center.y, 0.);
    glBegin(GL_POLYGON);
    glVertex2d(- halfWidth, - halfHeight); // bottom left
    glVertex2d(- halfWidth, + halfHeight); // top left
    glVertex2d(+ halfWidth, + halfHeight); // bottom right
    glVertex2d(+ halfWidth, - halfHeight); // top right
    glEnd();
    glPopMatrix();

}

static void
drawEllipse(const OfxRGBColourD& color,
            const OfxPointD& center,
            const OfxPointD& targetRadius,
            bool hovered,
            int l)
{
    if (hovered) {
        glColor3f(1.f*l, 0.f*l, 0.f*l);
    } else {
        glColor3f((float)color.r*l, (float)color.g*l, (float)color.b*l);
    }

    glPushMatrix();
    //  center the oval at x_center, y_center
    glTranslatef((float)center.x, (float)center.y, 0.f);
    //  draw the oval using line segments
    glBegin(GL_LINE_LOOP);
    // we don't need to be pixel-perfect here, it's just an interact!
    // 40 segments is enough.
    for (int i = 0; i < 40; ++i) {
        double theta = i * 2 * OFX::ofxsPi() / 40.;
        glVertex2d(targetRadius.x * std::cos(theta), targetRadius.y * std::sin(theta));
    }
    glEnd();

    glPopMatrix();
}

static void
drawSkewBar(const OfxRGBColourD& color,
            const OfxPointD &center,
            const OfxPointD& pixelScale,
            double targetRadiusY,
            bool hovered,
            double angle,
            int l)
{
    if (hovered) {
        glColor3f(1.f*l, 0.f*l, 0.f*l);
    } else {
        glColor3f((float)color.r*l, (float)color.g*l, (float)color.b*l);
    }

    // we are not axis-aligned: use the mean pixel scale
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    double barHalfSize = targetRadiusY + 20. * meanPixelScale;

    glPushMatrix();
    glTranslatef((float)center.x, (float)center.y, 0.f);
    glRotated(angle, 0, 0, 1);

    glBegin(GL_LINES);
    glVertex2d(0., - barHalfSize);
    glVertex2d(0., + barHalfSize);

    if (hovered) {
        double arrowYPosition = targetRadiusY + 10. * meanPixelScale;
        double arrowXHalfSize = 10 * meanPixelScale;
        double arrowHeadOffsetX = 3 * meanPixelScale;
        double arrowHeadOffsetY = 3 * meanPixelScale;

        ///draw the central bar
        glVertex2d(- arrowXHalfSize, - arrowYPosition);
        glVertex2d(+ arrowXHalfSize, - arrowYPosition);

        ///left triangle
        glVertex2d(- arrowXHalfSize, -  arrowYPosition);
        glVertex2d(- arrowXHalfSize + arrowHeadOffsetX, - arrowYPosition + arrowHeadOffsetY);

        glVertex2d(- arrowXHalfSize,- arrowYPosition);
        glVertex2d(- arrowXHalfSize + arrowHeadOffsetX, - arrowYPosition - arrowHeadOffsetY);

        ///right triangle
        glVertex2d(+ arrowXHalfSize,- arrowYPosition);
        glVertex2d(+ arrowXHalfSize - arrowHeadOffsetX, - arrowYPosition + arrowHeadOffsetY);

        glVertex2d(+ arrowXHalfSize,- arrowYPosition);
        glVertex2d(+ arrowXHalfSize - arrowHeadOffsetX, - arrowYPosition - arrowHeadOffsetY);
    }
    glEnd();
    glPopMatrix();
}


static void
drawRotationBar(const OfxRGBColourD& color,
                const OfxPointD& pixelScale,
                double targetRadiusX,
                bool hovered,
                bool inverted,
                int l)
{
    // we are not axis-aligned
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    if (hovered) {
        glColor3f(1.f*l, 0.f*l, 0.f*l);
    } else {
        glColor3f(color.r*l, color.g*l, color.b*l);
    }

    double barExtra = 30. * meanPixelScale;
    glBegin(GL_LINES);
    glVertex2d(0., 0.);
    glVertex2d(0. + targetRadiusX + barExtra, 0.);
    glEnd();

    if (hovered) {
        double arrowCenterX = targetRadiusX + barExtra / 2.;

        ///draw an arrow slightly bended. This is an arc of circle of radius 5 in X, and 10 in Y.
        OfxPointD arrowRadius;
        arrowRadius.x = 5. * meanPixelScale;
        arrowRadius.y = 10. * meanPixelScale;

        glPushMatrix();
        //  center the oval at x_center, y_center
        glTranslatef((float)arrowCenterX, 0.f, 0);
        //  draw the oval using line segments
        glBegin(GL_LINE_STRIP);
        glVertex2d(0, arrowRadius.y);
        glVertex2d(arrowRadius.x, 0.);
        glVertex2d(0, -arrowRadius.y);
        glEnd();


        glBegin(GL_LINES);
        ///draw the top head
        glVertex2d(0., arrowRadius.y);
        glVertex2d(0., arrowRadius.y - 5. * meanPixelScale);

        glVertex2d(0., arrowRadius.y);
        glVertex2d(4. * meanPixelScale, arrowRadius.y - 3. * meanPixelScale); // 5^2 = 3^2+4^2

        ///draw the bottom head
        glVertex2d(0., -arrowRadius.y);
        glVertex2d(0., -arrowRadius.y + 5. * meanPixelScale);

        glVertex2d(0., -arrowRadius.y);
        glVertex2d(4. * meanPixelScale, -arrowRadius.y + 3. * meanPixelScale); // 5^2 = 3^2+4^2

        glEnd();

        glPopMatrix();
    }
    if (inverted) {
        double arrowXPosition = targetRadiusX + barExtra * 1.5;
        double arrowXHalfSize = 10 * meanPixelScale;
        double arrowHeadOffsetX = 3 * meanPixelScale;
        double arrowHeadOffsetY = 3 * meanPixelScale;

        glPushMatrix();
        glTranslatef((float)arrowXPosition, 0, 0);

        glBegin(GL_LINES);
        ///draw the central bar
        glVertex2d(- arrowXHalfSize, 0.);
        glVertex2d(+ arrowXHalfSize, 0.);

        ///left triangle
        glVertex2d(- arrowXHalfSize, 0.);
        glVertex2d(- arrowXHalfSize + arrowHeadOffsetX, arrowHeadOffsetY);

        glVertex2d(- arrowXHalfSize, 0.);
        glVertex2d(- arrowXHalfSize + arrowHeadOffsetX, -arrowHeadOffsetY);

        ///right triangle
        glVertex2d(+ arrowXHalfSize, 0.);
        glVertex2d(+ arrowXHalfSize - arrowHeadOffsetX, arrowHeadOffsetY);

        glVertex2d(+ arrowXHalfSize, 0.);
        glVertex2d(+ arrowXHalfSize - arrowHeadOffsetX, -arrowHeadOffsetY);
        glEnd();

        glRotated(90., 0., 0., 1.);

        glBegin(GL_LINES);
        ///draw the central bar
        glVertex2d(- arrowXHalfSize, 0.);
        glVertex2d(+ arrowXHalfSize, 0.);

        ///left triangle
        glVertex2d(- arrowXHalfSize, 0.);
        glVertex2d(- arrowXHalfSize + arrowHeadOffsetX, arrowHeadOffsetY);

        glVertex2d(- arrowXHalfSize, 0.);
        glVertex2d(- arrowXHalfSize + arrowHeadOffsetX, -arrowHeadOffsetY);

        ///right triangle
        glVertex2d(+ arrowXHalfSize, 0.);
        glVertex2d(+ arrowXHalfSize - arrowHeadOffsetX, arrowHeadOffsetY);

        glVertex2d(+ arrowXHalfSize, 0.);
        glVertex2d(+ arrowXHalfSize - arrowHeadOffsetX, -arrowHeadOffsetY);
        glEnd();

        glPopMatrix();
    }

}

// draw the interact
bool
TransformInteract::draw(const OFX::DrawArgs &args)
{
    const OfxPointD &pscale = args.pixelScale;
    const double time = args.time;
    OfxRGBColourD color = { 0.8, 0.8, 0.8 };
    getSuggestedColour(color);
    GLdouble projection[16];
    glGetDoublev( GL_PROJECTION_MATRIX, projection);
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    OfxPointD shadow; // how much to translate GL_PROJECTION to get exactly one pixel on screen
    shadow.x = 2./(projection[0] * viewport[2]);
    shadow.y = 2./(projection[5] * viewport[3]);

    OfxPointD center;
    OfxPointD translate;
    OfxPointD scaleParam;
    bool scaleUniform;
    double rotate;
    double skewX, skewY;
    int skewOrder;
    bool inverted;

    if (_mouseState == eReleased) {
        _center->getValueAtTime(time, center.x, center.y);
        _translate->getValueAtTime(time, translate.x, translate.y);
        _scale->getValueAtTime(time, scaleParam.x, scaleParam.y);
        _scaleUniform->getValueAtTime(time, scaleUniform);
        _rotate->getValueAtTime(time, rotate);
        _skewX->getValueAtTime(time, skewX);
        _skewY->getValueAtTime(time, skewY);
        _skewOrder->getValueAtTime(time, skewOrder);
        _invert->getValueAtTime(time, inverted);
    } else {
        center = _centerDrag;
        translate = _translateDrag;
        scaleParam = _scaleParamDrag;
        scaleUniform = _scaleUniformDrag;
        rotate = _rotateDrag;
        skewX = _skewXDrag;
        skewY = _skewYDrag;
        skewOrder = _skewOrderDrag;
        inverted = _invertedDrag;
    }

    OfxPointD targetCenter;
    getTargetCenter(center, translate, &targetCenter);

    OfxPointD scale;
    ofxsTransformGetScale(scaleParam, scaleUniform, &scale);

    OfxPointD targetRadius;
    getTargetRadius(scale, pscale, &targetRadius);

    OfxPointD left, right, bottom, top;
    getTargetPoints(targetCenter, targetRadius, &left, &bottom, &top, &right);


    GLdouble skewMatrix[16];
    skewMatrix[0] = (skewOrder ? 1. : (1.+skewX*skewY)); skewMatrix[1] = skewY; skewMatrix[2] = 0.; skewMatrix[3] = 0;
    skewMatrix[4] = skewX; skewMatrix[5] = (skewOrder ? (1.+skewX*skewY) : 1.); skewMatrix[6] = 0.; skewMatrix[7] = 0;
    skewMatrix[8] = 0.; skewMatrix[9] = 0.; skewMatrix[10] = 1.; skewMatrix[11] = 0;
    skewMatrix[12] = 0.; skewMatrix[13] = 0.; skewMatrix[14] = 0.; skewMatrix[15] = 1.;

    //glPushAttrib(GL_ALL_ATTRIB_BITS); // caller is responsible for protecting attribs

    //glDisable(GL_LINE_STIPPLE);
    glEnable(GL_LINE_SMOOTH);
    //glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
    glLineWidth(1.5f);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    // Draw everything twice
    // l = 0: shadow
    // l = 1: drawing
    for (int l = 0; l < 2; ++l) {
        // shadow (uses GL_PROJECTION)
        glMatrixMode(GL_PROJECTION);
        int direction = (l == 0) ? 1 : -1;
        // translate (1,-1) pixels
        glTranslated(direction * shadow.x, -direction * shadow.y, 0);
        glMatrixMode(GL_MODELVIEW); // Modelview should be used on Nuke

        glColor3f(color.r*l, color.g*l, color.b*l);

        glPushMatrix();
        glTranslated(targetCenter.x, targetCenter.y, 0.);

        glRotated(rotate, 0, 0., 1.);
        drawRotationBar(color, pscale, targetRadius.x, _mouseState == eDraggingRotationBar || _drawState == eRotationBarHovered, inverted, l);
        glMultMatrixd(skewMatrix);
        glTranslated(-targetCenter.x, -targetCenter.y, 0.);

        drawEllipse(color, targetCenter, targetRadius, _mouseState == eDraggingCircle || _drawState == eCircleHovered, l);

        // add 180 to the angle to draw the arrows on the other side. unfortunately, this requires knowing
        // the mouse position in the ellipse frame
        double flip = 0.;
        if (_drawState == eSkewXBarHoverered || _drawState == eSkewYBarHoverered) {
            double rot = OFX::ofxsToRadians(rotate);
            OFX::Matrix3x3 transformscale;
            transformscale = OFX::ofxsMatInverseTransformCanonical(0., 0., scale.x, scale.y, skewX, skewY, (bool)skewOrder, rot, targetCenter.x, targetCenter.y);

            OFX::Point3D previousPos;
            previousPos.x = _lastMousePos.x;
            previousPos.y = _lastMousePos.y;
            previousPos.z = 1.;
            previousPos = transformscale * previousPos;
            if (previousPos.z != 0) {
                previousPos.x /= previousPos.z;
                previousPos.y /= previousPos.z;
            }
            if ((_drawState == eSkewXBarHoverered && previousPos.y > targetCenter.y) ||
                (_drawState == eSkewYBarHoverered && previousPos.x > targetCenter.x)) {
                flip = 180.;
            }
        }
        drawSkewBar(color, targetCenter, pscale, targetRadius.y, _mouseState == eDraggingSkewXBar || _drawState == eSkewXBarHoverered, flip, l);
        drawSkewBar(color, targetCenter, pscale, targetRadius.x, _mouseState == eDraggingSkewYBar || _drawState == eSkewYBarHoverered, flip - 90., l);


        drawSquare(color, targetCenter, pscale, _mouseState == eDraggingTranslation || _mouseState == eDraggingCenter || _drawState == eCenterPointHovered, _modifierStateCtrl, l);
        drawSquare(color, left, pscale, _mouseState == eDraggingLeftPoint || _drawState == eLeftPointHovered, false, l);
        drawSquare(color, right, pscale, _mouseState == eDraggingRightPoint || _drawState == eRightPointHovered, false, l);
        drawSquare(color, top, pscale, _mouseState == eDraggingTopPoint || _drawState == eTopPointHovered, false, l);
        drawSquare(color, bottom, pscale, _mouseState == eDraggingBottomPoint || _drawState == eBottomPointHovered, false, l);

        glPopMatrix();
    }
    //glPopAttrib();

    return true;
}

static bool squareContains(const OFX::Point3D& pos,
                           const OfxRectD& rect,
                           double toleranceX= 0.,
                           double toleranceY = 0.)
{
    return (pos.x >= (rect.x1 - toleranceX) && pos.x < (rect.x2 + toleranceX)
            && pos.y >= (rect.y1 - toleranceY) && pos.y < (rect.y2 + toleranceY));
}

static bool isOnEllipseBorder(const OFX::Point3D& pos,
                              const OfxPointD& targetRadius,
                              const OfxPointD& targetCenter,
                              double epsilon = 0.1)
{

    double v = ((pos.x - targetCenter.x) * (pos.x - targetCenter.x) / (targetRadius.x * targetRadius.x) +
                (pos.y - targetCenter.y) * (pos.y - targetCenter.y) / (targetRadius.y * targetRadius.y));
    if (v <= (1. + epsilon) && v >= (1. - epsilon)) {
        return true;
    }
    return false;
}

static bool isOnSkewXBar(const OFX::Point3D& pos,
                         double targetRadiusY,
                         const OfxPointD& center,
                         const OfxPointD& pixelScale,
                         double tolerance)
{
    // we are not axis-aligned
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    double barHalfSize = targetRadiusY + (20. * meanPixelScale);
    if (pos.x >= (center.x - tolerance) && pos.x <= (center.x + tolerance) &&
        pos.y >= (center.y - barHalfSize - tolerance) && pos.y <= (center.y + barHalfSize + tolerance)) {
        return true;
    }

    return false;
}

static bool isOnSkewYBar(const OFX::Point3D& pos,
                         double targetRadiusX,
                         const OfxPointD& center,
                         const OfxPointD& pixelScale,
                         double tolerance)
{
    // we are not axis-aligned
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    double barHalfSize = targetRadiusX + (20. * meanPixelScale);
    if (pos.y >= (center.y - tolerance) && pos.y <= (center.y + tolerance) &&
        pos.x >= (center.x - barHalfSize - tolerance) && pos.x <= (center.x + barHalfSize + tolerance)) {
        return true;
    }

    return false;
}

static bool isOnRotationBar(const  OFX::Point3D& pos,
                            double targetRadiusX,
                            const OfxPointD& center,
                            const OfxPointD& pixelScale,
                            double tolerance)
{
    // we are not axis-aligned
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    double barExtra = 30. * meanPixelScale;
    if (pos.x >= (center.x - tolerance) && pos.x <= (center.x + targetRadiusX + barExtra + tolerance) &&
        pos.y >= (center.y  - tolerance) && pos.y <= (center.y + tolerance)) {
        return true;
    }

    return false;
}

static OfxRectD rectFromCenterPoint(const OfxPointD& center,
                                    const OfxPointD& pixelScale)
{
    // we are not axis-aligned
    double meanPixelScale = (pixelScale.x + pixelScale.y) / 2.;
    OfxRectD ret;
    ret.x1 = center.x - (POINT_SIZE / 2.) * meanPixelScale;
    ret.x2 = center.x + (POINT_SIZE / 2.) * meanPixelScale;
    ret.y1 = center.y - (POINT_SIZE / 2.) * meanPixelScale;
    ret.y2 = center.y + (POINT_SIZE / 2.) * meanPixelScale;
    return ret;
}


// round to the closest int, 1/10 int, etc
// this make parameter editing easier
// pscale is args.pixelScale.x / args.renderScale.x;
// pscale10 is the power of 10 below pscale
static double fround(double val, double pscale)
{
    double pscale10 = std::pow(10.,std::floor(std::log10(pscale)));
    return pscale10 * std::floor(val/pscale10 + 0.5);
}

// overridden functions from OFX::Interact to do things
bool TransformInteract::penMotion(const OFX::PenArgs &args)
{
    const OfxPointD &pscale = args.pixelScale;
    const double time = args.time;

    OfxPointD center;
    OfxPointD translate;
    OfxPointD scaleParam;
    bool scaleUniform;
    double rotate;
    double skewX, skewY;
    int skewOrder;
    bool inverted;

    if (_mouseState == eReleased) {
        _center->getValueAtTime(time, center.x, center.y);
        _translate->getValueAtTime(time, translate.x, translate.y);
        _scale->getValueAtTime(time, scaleParam.x, scaleParam.y);
        _scaleUniform->getValueAtTime(time, scaleUniform);
        _rotate->getValueAtTime(time, rotate);
        _skewX->getValueAtTime(time, skewX);
        _skewY->getValueAtTime(time, skewY);
        _skewOrder->getValueAtTime(time, skewOrder);
        _invert->getValueAtTime(time, inverted);
    } else {
        center = _centerDrag;
        translate = _translateDrag;
        scaleParam = _scaleParamDrag;
        scaleUniform = _scaleUniformDrag;
        rotate = _rotateDrag;
        skewX = _skewXDrag;
        skewY = _skewYDrag;
        skewOrder = _skewOrderDrag;
        inverted = _invertedDrag;
    }

    bool didSomething = false;
    bool centerChanged = false;
    bool translateChanged = false;
    bool scaleChanged = false;
    bool rotateChanged = false;
    bool skewXChanged = false;
    bool skewYChanged = false;

    OfxPointD targetCenter;
    getTargetCenter(center, translate, &targetCenter);

    OfxPointD scale;
    ofxsTransformGetScale(scaleParam, scaleUniform, &scale);

    OfxPointD targetRadius;
    getTargetRadius(scale, pscale, &targetRadius);

    OfxPointD left, right, bottom, top;
    getTargetPoints(targetCenter, targetRadius, &left, &bottom, &top, &right);

    OfxRectD centerPoint = rectFromCenterPoint(targetCenter, pscale);
    OfxRectD leftPoint = rectFromCenterPoint(left, pscale);
    OfxRectD rightPoint = rectFromCenterPoint(right, pscale);
    OfxRectD topPoint = rectFromCenterPoint(top, pscale);
    OfxRectD bottomPoint = rectFromCenterPoint(bottom, pscale);


    //double dx = args.penPosition.x - _lastMousePos.x;
    //double dy = args.penPosition.y - _lastMousePos.y;

    double rot = OFX::ofxsToRadians(rotate);


    OFX::Point3D penPos, prevPenPos, rotationPos, transformedPos, previousPos, currentPos;
    penPos.x = args.penPosition.x;
    penPos.y = args.penPosition.y;
    penPos.z = 1.;
    prevPenPos.x = _lastMousePos.x;
    prevPenPos.y = _lastMousePos.y;
    prevPenPos.z = 1.;

    OFX::Matrix3x3 rotation, transform, transformscale;
    ////for the rotation bar/translation/center dragging we dont use the same transform, we don't want to undo the rotation transform
    if (_mouseState != eDraggingTranslation && _mouseState != eDraggingCenter) {
        ///undo skew + rotation to the current position
        rotation = OFX::ofxsMatInverseTransformCanonical(0., 0., 1., 1., 0., 0., false, rot, targetCenter.x, targetCenter.y);
        transform = OFX::ofxsMatInverseTransformCanonical(0., 0., 1., 1., skewX, skewY, (bool)skewOrder, rot, targetCenter.x, targetCenter.y);
        transformscale = OFX::ofxsMatInverseTransformCanonical(0., 0., scale.x, scale.y, skewX, skewY, (bool)skewOrder, rot, targetCenter.x, targetCenter.y);
    } else {
        rotation = OFX::ofxsMatInverseTransformCanonical(0., 0., 1., 1., 0., 0., false, 0., targetCenter.x, targetCenter.y);
        transform = OFX::ofxsMatInverseTransformCanonical(0., 0., 1., 1., skewX, skewY, (bool)skewOrder, 0., targetCenter.x, targetCenter.y);
        transformscale = OFX::ofxsMatInverseTransformCanonical(0., 0., scale.x, scale.y, skewX, skewY, (bool)skewOrder, 0., targetCenter.x, targetCenter.y);
    }

    rotationPos = rotation * penPos;
    if (rotationPos.z != 0) {
        rotationPos.x /= rotationPos.z;
        rotationPos.y /= rotationPos.z;
    }

    transformedPos = transform * penPos;
    if (transformedPos.z != 0) {
        transformedPos.x /= transformedPos.z;
        transformedPos.y /= transformedPos.z;
    }

    previousPos = transformscale * prevPenPos;
    if (previousPos.z != 0) {
        previousPos.x /= previousPos.z;
        previousPos.y /= previousPos.z;
    }

    currentPos = transformscale * penPos;
    if (currentPos.z != 0) {
        currentPos.x /= currentPos.z;
        currentPos.y /= currentPos.z;
    }

    if (_mouseState == eReleased) {
        // we are not axis-aligned
        double meanPixelScale = (pscale.x + pscale.y) / 2.;
        double hoverTolerance = (POINT_SIZE / 2.) * meanPixelScale;
        if (squareContains(transformedPos, centerPoint)) {
            _drawState = eCenterPointHovered;
            didSomething = true;
        } else if (squareContains(transformedPos, leftPoint)) {
            _drawState = eLeftPointHovered;
            didSomething = true;
        } else if (squareContains(transformedPos, rightPoint)) {
            _drawState = eRightPointHovered;
            didSomething = true;
        } else if (squareContains(transformedPos, topPoint)) {
            _drawState = eTopPointHovered;
            didSomething = true;
        } else if (squareContains(transformedPos, bottomPoint)) {
            _drawState = eBottomPointHovered;
            didSomething = true;
        } else if (isOnEllipseBorder(transformedPos, targetRadius, targetCenter)) {
            _drawState = eCircleHovered;
            didSomething = true;
        } else if (isOnRotationBar(rotationPos, targetRadius.x, targetCenter, pscale, hoverTolerance)) {
            _drawState = eRotationBarHovered;
            didSomething = true;
        } else if (isOnSkewXBar(transformedPos, targetRadius.y, targetCenter, pscale, hoverTolerance)) {
            _drawState = eSkewXBarHoverered;
            didSomething = true;
        } else if (isOnSkewYBar(transformedPos, targetRadius.x, targetCenter, pscale, hoverTolerance)) {
            _drawState = eSkewYBarHoverered;
            didSomething = true;
        } else {
            _drawState = eInActive;
        }
    } else if (_mouseState == eDraggingCircle) {
        double minX,minY,maxX,maxY;
        _scale->getRange(minX, minY, maxX, maxY);

        // we need to compute the backtransformed points with the scale

        // the scale ratio is the ratio of distances to the center
        double prevDistSq = (targetCenter.x - previousPos.x)*(targetCenter.x - previousPos.x) + (targetCenter.y - previousPos.y)*(targetCenter.y - previousPos.y);
        if (prevDistSq != 0.) {
            const double distSq = (targetCenter.x - currentPos.x)*(targetCenter.x - currentPos.x) + (targetCenter.y - currentPos.y)*(targetCenter.y - currentPos.y);
            const double distRatio = std::sqrt(distSq/prevDistSq);
            scale.x *= distRatio;
            scale.y *= distRatio;
            //_scale->setValue(scale.x, scale.y);
            scaleChanged = true;
        }
    } else if (_mouseState == eDraggingLeftPoint || _mouseState == eDraggingRightPoint) {
        // avoid division by zero
        if (targetCenter.x != previousPos.x) {
            double minX,minY,maxX,maxY;
            _scale->getRange(minX, minY, maxX, maxY);
            const double scaleRatio = (targetCenter.x - currentPos.x)/(targetCenter.x - previousPos.x);
            OfxPointD newScale;
            newScale.x = scale.x * scaleRatio;
            newScale.x = std::max(minX, std::min(newScale.x, maxX));
            newScale.y = scaleUniform ? newScale.x : scale.y;
            scale = newScale;
            //_scale->setValue(scale.x, scale.y);
            scaleChanged = true;
        }
    } else if (_mouseState == eDraggingTopPoint || _mouseState == eDraggingBottomPoint) {
        // avoid division by zero
        if (targetCenter.y != previousPos.y) {
            double minX,minY,maxX,maxY;
            _scale->getRange(minX, minY, maxX, maxY);
            const double scaleRatio = (targetCenter.y - currentPos.y)/(targetCenter.y - previousPos.y);
            OfxPointD newScale;
            newScale.y = scale.y * scaleRatio;
            newScale.y = std::max(minY, std::min(newScale.y, maxY));
            newScale.x = scaleUniform ? newScale.y : scale.x;
            scale = newScale;
            //_scale->setValue(scale.x, scale.y);
            scaleChanged = true;
        }
    } else if (_mouseState == eDraggingTranslation) {
        double dx = args.penPosition.x - _lastMousePos.x;
        double dy = args.penPosition.y - _lastMousePos.y;

        if (_orientation == eOrientationNotSet && _modifierStateShift > 0) {
            _orientation = std::abs(dx) > std::abs(dy) ? eOrientationHorizontal : eOrientationVertical;
        }

        dx = _orientation == eOrientationVertical ? 0 : dx;
        dy = _orientation == eOrientationHorizontal ? 0 : dy;
        double newx = translate.x + dx;
        double newy = translate.y + dy;
        // round newx/y to the closest int, 1/10 int, etc
        // this make parameter editing easier
        newx = fround(newx, pscale.x);
        newy = fround(newy, pscale.y);
        translate.x = newx;
        translate.y = newy;
        //_translate->setValue(translate.x, translate.y);
        translateChanged = true;
    } else if (_mouseState == eDraggingCenter) {
        OfxPointD currentCenter = center;
        OFX::Matrix3x3 R = ofxsMatScale(1. / scale.x, 1. / scale.y) * ofxsMatSkewXY(-skewX, -skewY, !skewOrder) * ofxsMatRotation(rot);

        double dx = args.penPosition.x - _lastMousePos.x;
        double dy = args.penPosition.y - _lastMousePos.y;

        if (_orientation == eOrientationNotSet && _modifierStateShift > 0) {
            _orientation = std::abs(dx) > std::abs(dy) ? eOrientationHorizontal : eOrientationVertical;
        }

        dx = _orientation == eOrientationVertical ? 0 : dx;
        dy = _orientation == eOrientationHorizontal ? 0 : dy;

        OFX::Point3D dRot;
        dRot.x = dx;
        dRot.y = dy;
        dRot.z = 1.;
        dRot = R * dRot;
        if (dRot.z != 0) {
            dRot.x /= dRot.z;
            dRot.y /= dRot.z;
        }
        double dxrot = dRot.x;
        double dyrot = dRot.y;
        double newx = currentCenter.x + dxrot;
        double newy = currentCenter.y + dyrot;
        // round newx/y to the closest int, 1/10 int, etc
        // this make parameter editing easier
        newx = fround(newx, pscale.x);
        newy = fround(newy, pscale.y);
        center.x = newx;
        center.y = newy;
        //_plugin->beginEditBlock("setCenter");
        //_center->setValue(center.x, center.y);
        centerChanged = true;
        // recompute dxrot,dyrot after rounding
        double det = ofxsMatDeterminant(R);
        if (det != 0.) {
            OFX::Matrix3x3 Rinv = ofxsMatInverse(R, det);

            dxrot = newx - currentCenter.x;
            dyrot = newy - currentCenter.y;
            dRot.x = dxrot;
            dRot.y = dyrot;
            dRot.z = 1;
            dRot = Rinv * dRot;
            if (dRot.z != 0) {
                dRot.x /= dRot.z;
                dRot.y /= dRot.z;
            }
            dx = dRot.x;
            dy = dRot.y;
            OfxPointD newTranslation;
            newTranslation.x = translate.x + dx - dxrot;
            newTranslation.y = translate.y + dy - dyrot;
            translate = newTranslation;
            //_translate->setValue(translate.x, translate.y);
            translateChanged = true;
        }
        //_plugin->endEditBlock();
    } else if (_mouseState == eDraggingRotationBar) {
        OfxPointD diffToCenter;
        ///the current mouse position (untransformed) is doing has a certain angle relative to the X axis
        ///which can be computed by : angle = arctan(opposite / adjacent)
        diffToCenter.y = rotationPos.y - targetCenter.y;
        diffToCenter.x = rotationPos.x - targetCenter.x;
        double angle = std::atan2(diffToCenter.y, diffToCenter.x);
        double angledegrees = rotate + OFX::ofxsToDegrees(angle);
        double closest90 = 90. * std::floor((angledegrees + 45.)/90.);
        if (std::fabs(angledegrees - closest90) < 5.) {
            // snap to closest multiple of 90.
            angledegrees = closest90;
        }
        rotate = angledegrees;
        //_rotate->setValue(rotate);
        rotateChanged = true;

    } else if (_mouseState == eDraggingSkewXBar) {
        // avoid division by zero
        if (scale.y != 0. && targetCenter.y != previousPos.y) {
            const double addSkew = (scale.x/scale.y)*(currentPos.x - previousPos.x)/(currentPos.y - targetCenter.y);
            skewX = skewX + addSkew;
            //_skewX->setValue(skewX);
            skewXChanged = true;
        }
    } else if (_mouseState == eDraggingSkewYBar) {
        // avoid division by zero
        if (scale.x != 0. && targetCenter.x != previousPos.x) {
            const double addSkew = (scale.y/scale.x)*(currentPos.y - previousPos.y)/(currentPos.x - targetCenter.x);
            skewY = skewY + addSkew;
            //_skewY->setValue(skewY + addSkew);
            skewYChanged = true;
        }
    } else {
        assert(false);
    }

    _centerDrag = center;
    _translateDrag = translate;
    _scaleParamDrag = scale;
    _scaleUniformDrag = scaleUniform;
    _rotateDrag = rotate;
    _skewXDrag = skewX;
    _skewYDrag = skewY;
    _skewOrderDrag = skewOrder;
    _invertedDrag = inverted;

    bool valuesChanged = (centerChanged || translateChanged || scaleChanged || rotateChanged || skewXChanged || skewYChanged);

    if (_mouseState != eReleased && _interactiveDrag && valuesChanged) {
        // no need to redraw overlay since it is slave to the paramaters
        _plugin->beginEditBlock("setTransform");
        if (centerChanged) {
            _center->setValue(center.x, center.y);
        }
        if (translateChanged) {
            _translate->setValue(translate.x, translate.y);
        }
        if (scaleChanged) {
            _scale->setValue(scale.x, scale.y);
        }
        if (rotateChanged) {
            _rotate->setValue(rotate);
        }
        if (skewXChanged) {
            _skewX->setValue(skewX);
        }
        if (skewYChanged) {
            _skewY->setValue(skewY);
        }
        _plugin->endEditBlock();
    } else if (didSomething || valuesChanged) {
        _effect->redrawOverlays();
    }

    _lastMousePos = args.penPosition;

    return didSomething || valuesChanged;
}

bool TransformInteract::penDown(const OFX::PenArgs &args)
{
    using OFX::Matrix3x3;

    const OfxPointD &pscale = args.pixelScale;
    const double time = args.time;

    OfxPointD center;
    OfxPointD translate;
    OfxPointD scaleParam;
    bool scaleUniform;
    double rotate;
    double skewX, skewY;
    int skewOrder;
    bool inverted;

    if (_mouseState == eReleased) {
        _center->getValueAtTime(time, center.x, center.y);
        _translate->getValueAtTime(time, translate.x, translate.y);
        _scale->getValueAtTime(time, scaleParam.x, scaleParam.y);
        _scaleUniform->getValueAtTime(time, scaleUniform);
        _rotate->getValueAtTime(time, rotate);
        _skewX->getValueAtTime(time, skewX);
        _skewY->getValueAtTime(time, skewY);
        _skewOrder->getValueAtTime(time, skewOrder);
        _invert->getValueAtTime(time, inverted);
        if (_interactive) {
            _interactive->getValueAtTime(args.time, _interactiveDrag);
        }
    } else {
        center = _centerDrag;
        translate = _translateDrag;
        scaleParam = _scaleParamDrag;
        scaleUniform = _scaleUniformDrag;
        rotate = _rotateDrag;
        skewX = _skewXDrag;
        skewY = _skewYDrag;
        skewOrder = _skewOrderDrag;
        inverted = _invertedDrag;
    }

    OfxPointD targetCenter;
    getTargetCenter(center, translate, &targetCenter);

    OfxPointD scale;
    ofxsTransformGetScale(scaleParam, scaleUniform, &scale);

    OfxPointD targetRadius;
    getTargetRadius(scale, pscale, &targetRadius);

    OfxPointD left, right, bottom, top;
    getTargetPoints(targetCenter, targetRadius, &left, &bottom, &top, &right);

    OfxRectD centerPoint = rectFromCenterPoint(targetCenter, pscale);
    OfxRectD leftPoint = rectFromCenterPoint(left, pscale);
    OfxRectD rightPoint = rectFromCenterPoint(right, pscale);
    OfxRectD topPoint = rectFromCenterPoint(top, pscale);
    OfxRectD bottomPoint = rectFromCenterPoint(bottom, pscale);


    OFX::Point3D transformedPos, rotationPos;
    transformedPos.x = args.penPosition.x;
    transformedPos.y = args.penPosition.y;
    transformedPos.z = 1.;

    double rot = OFX::ofxsToRadians(rotate);

    ///now undo skew + rotation to the current position
    OFX::Matrix3x3 rotation, transform;
    rotation = OFX::ofxsMatInverseTransformCanonical(0., 0., 1., 1., 0., 0., false, rot, targetCenter.x, targetCenter.y);
    transform = OFX::ofxsMatInverseTransformCanonical(0., 0., 1., 1., skewX, skewY, (bool)skewOrder, rot, targetCenter.x, targetCenter.y);

    rotationPos = rotation * transformedPos;
    if (rotationPos.z != 0) {
        rotationPos.x /= rotationPos.z;
        rotationPos.y /= rotationPos.z;
    }
    transformedPos = transform * transformedPos;
    if (transformedPos.z != 0) {
        transformedPos.x /= transformedPos.z;
        transformedPos.y /= transformedPos.z;
    }

    _orientation = eOrientationAllDirections;

    double pressToleranceX = 5 * pscale.x;
    double pressToleranceY = 5 * pscale.y;

    bool didSomething = false;
    if (squareContains(transformedPos, centerPoint,pressToleranceX,pressToleranceY)) {
        _mouseState = _modifierStateCtrl ? eDraggingCenter : eDraggingTranslation;
        if (_modifierStateShift > 0) {
            _orientation = eOrientationNotSet;
        }
        didSomething = true;

    } else if (squareContains(transformedPos, leftPoint,pressToleranceX,pressToleranceY)) {
        _mouseState = eDraggingLeftPoint;
        didSomething = true;

    } else if (squareContains(transformedPos, rightPoint,pressToleranceX,pressToleranceY)) {
        _mouseState = eDraggingRightPoint;
        didSomething = true;

    } else if (squareContains(transformedPos, topPoint,pressToleranceX,pressToleranceY)) {
        _mouseState = eDraggingTopPoint;
        didSomething = true;

    } else if (squareContains(transformedPos, bottomPoint,pressToleranceX,pressToleranceY)) {
        _mouseState = eDraggingBottomPoint;
        didSomething = true;

    } else if (isOnEllipseBorder(transformedPos, targetRadius, targetCenter)) {
        _mouseState = eDraggingCircle;
        didSomething = true;

    } else if (isOnRotationBar(rotationPos, targetRadius.x, targetCenter, pscale, pressToleranceY)) {
        _mouseState = eDraggingRotationBar;
        didSomething = true;

    } else if (isOnSkewXBar(transformedPos, targetRadius.y, targetCenter, pscale, pressToleranceY)) {
        _mouseState = eDraggingSkewXBar;
        didSomething = true;

    } else if (isOnSkewYBar(transformedPos, targetRadius.x, targetCenter, pscale, pressToleranceX)) {
        _mouseState = eDraggingSkewYBar;
        didSomething = true;

    } else {
        _mouseState = eReleased;
    }

    _lastMousePos = args.penPosition;

    _centerDrag = center;
    _translateDrag = translate;
    _scaleParamDrag = scaleParam;
    _scaleUniformDrag = scaleUniform;
    _rotateDrag = rotate;
    _skewXDrag = skewX;
    _skewYDrag = skewY;
    _skewOrderDrag = skewOrder;
    _invertedDrag = inverted;

    if (didSomething) {
        _effect->redrawOverlays();
    }

    return didSomething;
}

bool TransformInteract::penUp(const OFX::PenArgs &args)
{
    bool ret = _mouseState != eReleased;

    if (!_interactiveDrag && _mouseState != eReleased) {
        // no need to redraw overlay since it is slave to the paramaters
        _plugin->beginEditBlock("setTransform");
        _center->setValue(_centerDrag.x, _centerDrag.y);
        _translate->setValue(_translateDrag.x, _translateDrag.y);
        _scale->setValue(_scaleParamDrag.x, _scaleParamDrag.y);
        _rotate->setValue(_rotateDrag);
        _skewX->setValue(_skewXDrag);
        _skewY->setValue(_skewYDrag);
        _plugin->endEditBlock();
    } else if (_mouseState != eReleased) {
        _effect->redrawOverlays();
    }

    _mouseState = eReleased;
    _lastMousePos = args.penPosition;

    return ret;
}

// keyDown just updates the modifier state
bool TransformInteract::keyDown(const OFX::KeyArgs &args)
{
    // Note that on the Mac:
    // cmd/apple/cloverleaf is kOfxKey_Control_L
    // ctrl is kOfxKey_Meta_L
    // alt/option is kOfxKey_Alt_L
    bool mustRedraw = false;

    // the two control keys may be pressed consecutively, be aware about this
    if (args.keySymbol == kOfxKey_Control_L || args.keySymbol == kOfxKey_Control_R) {
        mustRedraw = _modifierStateCtrl == 0;
        ++_modifierStateCtrl;
    }
    if (args.keySymbol == kOfxKey_Shift_L || args.keySymbol == kOfxKey_Shift_R) {
        mustRedraw = _modifierStateShift == 0;
        ++_modifierStateShift;
        if (_modifierStateShift > 0) {
            _orientation = eOrientationNotSet;
        }
    }
    if (mustRedraw) {
        _effect->redrawOverlays();
    }
    //std::cout << std::hex << args.keySymbol << std::endl;
    // modifiers are not "caught"
    return false;
}

// keyUp just updates the modifier state
bool TransformInteract::keyUp(const OFX::KeyArgs &args)
{
    bool mustRedraw = false;

    if (args.keySymbol == kOfxKey_Control_L || args.keySymbol == kOfxKey_Control_R) {
        // we may have missed a keypress
        if (_modifierStateCtrl > 0) {
            --_modifierStateCtrl;
            mustRedraw = _modifierStateCtrl == 0;
        }
    }
    if (args.keySymbol == kOfxKey_Shift_L || args.keySymbol == kOfxKey_Shift_R) {
        if (_modifierStateShift > 0) {
            --_modifierStateShift;
            mustRedraw = _modifierStateShift == 0;
        }
        if (_modifierStateShift == 0) {
            _orientation = eOrientationAllDirections;
        }
    }
    if (mustRedraw) {
        _effect->redrawOverlays();
    }
    // modifiers are not "caught"
    return false;
}

/** @brief Called when the interact is loses input focus */
void TransformInteract::loseFocus(const FocusArgs &/*args*/)
{
    // reset the modifiers state
    _modifierStateCtrl = 0;
    _modifierStateShift = 0;
    _interactiveDrag = false;
    _mouseState = eReleased;
    _drawState = eInActive;
}
