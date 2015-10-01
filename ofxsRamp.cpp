/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of openfx-supportext <https://github.com/devernay/openfx-supportext>,
 * Copyright (C) 2015 INRIA
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
 * OFX generic rectangle interact with 4 corner points + center point and 4 mid-points.
 * You can use it to define any rectangle in an image resizable by the user.
 */

#include "ofxsRamp.h"
#include <cmath>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

using OFX::RampInteract;

static inline
void
crossProd(const Ofx3DPointD& u,
          const Ofx3DPointD& v,
          Ofx3DPointD* w)
{
    w->x = u.y*v.z - u.z*v.y;
    w->y = u.z*v.x - u.x*v.z;
    w->z = u.x*v.y - u.y*v.x;
}

bool
RampInteract::draw(const DrawArgs &args)
{
    int type_i;
    _type->getValueAtTime(args.time, type_i);
    RampTypeEnum type = (RampTypeEnum)type_i;
    bool noramp = (type == eRampTypeNone);
    if (noramp) {
        return false;
    }
    OfxRGBColourD color = { 0.8, 0.8, 0.8 };
    getSuggestedColour(color);
    const OfxPointD &pscale = args.pixelScale;
    GLdouble projection[16];
    glGetDoublev( GL_PROJECTION_MATRIX, projection);
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    OfxPointD shadow; // how much to translate GL_PROJECTION to get exactly one pixel on screen
    shadow.x = 2./(projection[0] * viewport[2]);
    shadow.y = 2./(projection[5] * viewport[3]);

    OfxPointD p[2];
    if (_state == eInteractStateIdle) {
        _point0->getValueAtTime(args.time, p[0].x, p[0].y);
        _point1->getValueAtTime(args.time, p[1].x, p[1].y);
    } else {
        p[0] = _point0DragPos;
        p[1] = _point1DragPos;
    }
    
    ///Clamp points to the rod
    OfxRectD rod = _effect->getRegionOfDefinitionForInteract(args.time);

    // A line is represented by a 3-vector (a,b,c), and its equation is (a,b,c).(x,y,1)=0
    // The intersection of two lines is given by their cross-product: (wx,wy,w) = (a,b,c)x(a',b',c').
    // The line passing through 2 points is obtained by their cross-product: (a,b,c) = (x,y,1)x(x',y',1)
    // The two lines passing through p0 and p1 and orthogonal to p0p1 are:
    // (p1.x - p0.x, p1.y - p0.y, -p0.x*(p1.x-p0.x) - p0.y*(p1.y-p0.y)) passing through p0
    // (p1.x - p0.x, p1.y - p0.y, -p1.x*(p1.x-p0.x) - p1.y*(p1.y-p0.y)) passing through p1
    // the four lines defining the RoD are:
    // (1,0,-x1) [x=x1]
    // (1,0,-x2) [x=x2]
    // (0,1,-y1) [x=y1]
    // (0,1,-y2) [y=y2]
    const Ofx3DPointD linex1 = {1, 0, -rod.x1};
    const Ofx3DPointD linex2 = {1, 0, -rod.x2};
    const Ofx3DPointD liney1 = {0, 1, -rod.y1};
    const Ofx3DPointD liney2 = {0, 1, -rod.y2};

    Ofx3DPointD line[2];
    OfxPointD pline1[2];
    OfxPointD pline2[2];

    // line passing through p0
    line[0].x = p[1].x - p[0].x;
    line[0].y = p[1].y - p[0].y;
    line[0].z = -p[0].x*(p[1].x-p[0].x) - p[0].y*(p[1].y-p[0].y);
    // line passing through p1
    line[1].x = p[1].x - p[0].x;
    line[1].y = p[1].y - p[0].y;
    line[1].z = -p[1].x*(p[1].x-p[0].x) - p[1].y*(p[1].y-p[0].y);
    // for each line...
    for (int i = 0; i < 2; ++i) {
        // compute the intersection with the four lines
        Ofx3DPointD interx1, interx2, intery1, intery2;

        crossProd(line[i], linex1, &interx1);
        crossProd(line[i], linex2, &interx2);
        crossProd(line[i], liney1, &intery1);
        crossProd(line[i], liney2, &intery2);
        if (interx1.z != 0. && interx2.z != 0.) {
            // initialize pline1 to the intersection with x=x1, pline2 with x=x2
            pline1[i].x = interx1.x/interx1.z;
            pline1[i].y = interx1.y/interx1.z;
            pline2[i].x = interx2.x/interx2.z;
            pline2[i].y = interx2.y/interx2.z;
            if ((pline1[i].y > rod.y2 && pline2[i].y > rod.y2) ||
                (pline1[i].y < rod.y1 && pline2[i].y < rod.y1)) {
                // line doesn't intersect rectangle, don't draw it.
                pline1[i].x = p[i].x;
                pline1[i].y = p[i].y;
                pline2[i].x = p[i].x;
                pline2[i].y = p[i].y;
            } else if (pline1[i].y < pline2[i].y) {
                // y is an increasing function of x, test the two other endpoints
                if (intery1.z != 0. && intery1.x/intery1.z > pline1[i].x) {
                    pline1[i].x = intery1.x/intery1.z;
                    pline1[i].y = intery1.y/intery1.z;
                }
                if (intery2.z != 0. && intery2.x/intery2.z < pline2[i].x) {
                    pline2[i].x = intery2.x/intery2.z;
                    pline2[i].y = intery2.y/intery2.z;
                }
            } else {
                // y is an decreasing function of x, test the two other endpoints
                if (intery2.z != 0. && intery2.x/intery2.z > pline1[i].x) {
                    pline1[i].x = intery2.x/intery2.z;
                    pline1[i].y = intery2.y/intery2.z;
                }
                if (intery1.z != 0. && intery1.x/intery1.z < pline2[i].x) {
                    pline2[i].x = intery1.x/intery1.z;
                    pline2[i].y = intery1.y/intery1.z;
                }
            }
        } else {
            // initialize pline1 to the intersection with y=y1, pline2 with y=y2
            pline1[i].x = intery1.x/intery1.z;
            pline1[i].y = intery1.y/intery1.z;
            pline2[i].x = intery2.x/intery2.z;
            pline2[i].y = intery2.y/intery2.z;
            if ((pline1[i].x > rod.x2 && pline2[i].x > rod.x2) ||
                (pline1[i].x < rod.x1 && pline2[i].x < rod.x1)) {
                // line doesn't intersect rectangle, don't draw it.
                pline1[i].x = p[i].x;
                pline1[i].y = p[i].y;
                pline2[i].x = p[i].x;
                pline2[i].y = p[i].y;
            }
        }
    }

    //glPushAttrib(GL_ALL_ATTRIB_BITS); // caller is responsible for protecting attribs

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glHint(GL_LINE_SMOOTH_HINT,GL_DONT_CARE);
    glLineWidth(1.5f);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    glPointSize(POINT_SIZE);
    
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
        
        for (int i = 0; i < 2; ++i) {
            bool dragging = _state == (i == 0 ? eInteractStateDraggingPoint0 : eInteractStateDraggingPoint1);
            glBegin(GL_POINTS);
            if (dragging) {
                glColor3f(0.f*l, 1.f*l, 0.f*l);
            } else {
                glColor3f((float)color.r*l, (float)color.g*l, (float)color.b*l);
            }
            glVertex2d(p[i].x, p[i].y);
            glEnd();

            glLineStipple(2, 0xAAAA);
            glEnable(GL_LINE_STIPPLE);
            glBegin(GL_LINES);
            glColor3f((float)color.r*l, (float)color.g*l, (float)color.b*l);
            glVertex2d(pline1[i].x, pline1[i].y);
            glVertex2d(pline2[i].x, pline2[i].y);
            glEnd();

            double xoffset = 5 * pscale.x;
            double yoffset = 5 * pscale.y;
            TextRenderer::bitmapString(p[i].x + xoffset, p[i].y + yoffset, i == 0 ? kParamRampPoint0Label : kParamRampPoint1Label);
        }
    }

    //glPopAttrib();

    return true;
}

static bool isNearby(const OfxPointD& p, double x, double y, double tolerance, const OfxPointD& pscale)
{
    return std::fabs(p.x-x) <= tolerance*pscale.x &&  std::fabs(p.y-y) <= tolerance*pscale.y;
}


bool
RampInteract::penMotion(const PenArgs &args)
{
    int type_i;
    _type->getValueAtTime(args.time, type_i);
    RampTypeEnum type = (RampTypeEnum)type_i;
    bool noramp = (type == eRampTypeNone);
    if (noramp) {
        return false;
    }

    const OfxPointD &pscale = args.pixelScale;

    OfxPointD p0,p1;
    if (_state != eInteractStateIdle) {
        p0 = _point0DragPos;
        p1 = _point1DragPos;
    } else {
        _point0->getValueAtTime(args.time, p0.x, p0.y);
        _point1->getValueAtTime(args.time, p1.x, p1.y);
    }
    bool valuesChanged = false;
    
    OfxPointD delta;
    delta.x = args.penPosition.x - _lastMousePos.x;
    delta.y = args.penPosition.y - _lastMousePos.y;
    
    if (_state == eInteractStateDraggingPoint0) {
        _point0DragPos.x += delta.x;
        _point0DragPos.y += delta.y;
        valuesChanged = true;

    } else if (_state == eInteractStateDraggingPoint1) {
        _point1DragPos.x += delta.x;
        _point1DragPos.y += delta.y;
        valuesChanged = true;
    }

    if (_state != eInteractStateIdle && _interactiveDrag && valuesChanged) {
        if (_state == eInteractStateDraggingPoint0) {
            _point0->setValue(fround(_point0DragPos.x, pscale.x), fround(_point0DragPos.y, pscale.y));
        } else if (_state == eInteractStateDraggingPoint1) {
            _point1->setValue(fround(_point1DragPos.x, pscale.x), fround(_point1DragPos.y, pscale.y));
        }
    }

    if (valuesChanged) {
        _effect->redrawOverlays();
    }

    _lastMousePos = args.penPosition;

    return valuesChanged;
}

bool
RampInteract::penDown(const PenArgs &args)
{
    int type_i;
    _type->getValueAtTime(args.time, type_i);
    RampTypeEnum type = (RampTypeEnum)type_i;
    bool noramp = (type == eRampTypeNone);
    if (noramp) {
        return false;
    }

    const OfxPointD &pscale = args.pixelScale;

    OfxPointD p0,p1;
    if (_state != eInteractStateIdle) {
        p0 = _point0DragPos;
        p1 = _point1DragPos;
    } else {
        _point0->getValueAtTime(args.time, p0.x, p0.y);
        _point1->getValueAtTime(args.time, p1.x, p1.y);
        _interactive->getValueAtTime(args.time, _interactiveDrag);
    }

    bool didSomething = false;
    
    if (isNearby(args.penPosition, p0.x, p0.y, POINT_TOLERANCE, pscale)) {
        _state = eInteractStateDraggingPoint0;
        didSomething = true;
    } else if (isNearby(args.penPosition, p1.x, p1.y, POINT_TOLERANCE, pscale)) {
        _state = eInteractStateDraggingPoint1;
        didSomething = true;
    } else {
        _state = eInteractStateIdle;
    }
    
    _point0DragPos = p0;
    _point1DragPos = p1;
    _lastMousePos = args.penPosition;

    if (didSomething) {
        _effect->redrawOverlays();
    }

    return didSomething;
}

bool
RampInteract::penUp(const PenArgs &args)
{
    int type_i;
    _type->getValueAtTime(args.time, type_i);
    RampTypeEnum type = (RampTypeEnum)type_i;
    bool noramp = (type == eRampTypeNone);
    if (noramp) {
        return false;
    }

    bool didSomething = false;
    const OfxPointD &pscale = args.pixelScale;

    if (!_interactiveDrag && _state != eInteractStateIdle) {
        if (_state == eInteractStateDraggingPoint0) {
            // round newx/y to the closest int, 1/10 int, etc
            // this make parameter editing easier

            _point0->setValue(fround(_point0DragPos.x, pscale.x), fround(_point0DragPos.y, pscale.y));
            didSomething = true;
        } else if (_state == eInteractStateDraggingPoint1) {
            _point1->setValue(fround(_point1DragPos.x, pscale.x), fround(_point1DragPos.y, pscale.y));
            didSomething = true;
        }
    } else  if (_state != eInteractStateIdle) {
        _effect->redrawOverlays();
    }
    
    _state = eInteractStateIdle;
    
    return didSomething;
}

/** @brief Called when the interact is loses input focus */
void
RampInteract::loseFocus(const FocusArgs &/*args*/)
{
    _interactiveDrag = false;
    _state = eInteractStateIdle;
}

template<RampTypeEnum type>
double
rampFunc(double t)
{
    if (t >= 1. || type == eRampTypeNone) {
        t = 1.;
    } else if (t <= 0) {
        t = 0.;
    } else {
        // from http://www.comp-fu.com/2012/01/nukes-smooth-ramp-functions/
        // linear
        //y = x
        // plinear: perceptually linear in rec709
        //y = pow(x, 3)
        // smooth: traditional smoothstep
        //y = x*x*(3 - 2*x)
        // smooth0: Catmull-Rom spline, smooth start, linear end
        //y = x*x*(2 - x)
        // smooth1: Catmull-Rom spline, linear start, smooth end
        //y = x*(1 + x*(1 - x))
        switch (type) {
            case eRampTypeLinear:
                break;
            case eRampTypePLinear:
                // plinear: perceptually linear in rec709
                t = t*t*t;
                break;
            case eRampTypeEaseIn:
                //t *= t; // old version, end of curve is too sharp
                // smooth0: Catmull-Rom spline, smooth start, linear end
                t = t*t*(2-t);
                break;
            case eRampTypeEaseOut:
                //t = - t * (t - 2); // old version, start of curve is too sharp
                // smooth1: Catmull-Rom spline, linear start, smooth end
                t = t*(1 + t*(1 - t));
                break;
            case eRampTypeSmooth:
                /*
                  t *= 2.;
                  if (t < 1) {
                  t = t * t / (2.);
                  } else {
                  --t;
                  t =  -0.5 * (t * (t - 2) - 1);
                  }
                */
                // smooth: traditional smoothstep
                t = t*t*(3 - 2*t);
                break;
            case eRampTypeNone:
                t = 1.;
                break;
            default:
                break;
        }
    }
    return t;
}

template<RampTypeEnum type>
double
rampFunc(const OfxPointD& p0, const OfxPointD& p1, const OfxPointD& p)
{
    double t = (p.x - p0.x) * nx + (p.y - p0.y) * ny;
    return rampFunc<type>(t);
}

double
rampFunc(const OfxPointD& p0, const OfxPointD& p1, RampTypeEnum type, const OfxPointD& p)
{
    double t = (p.x - p0.x) * nx + (p.y - p0.y) * ny;
    switch (type) {
        case eRampTypeLinear:
            return rampFunc<eRampTypeLinear>(t);
            break;
        case eRampTypePLinear:
            return rampFunc<eRampTypePLinear>(t);
            break;
        case eRampTypeEaseIn:
            return rampFunc<eRampTypeEaseIn>(t);
            break;
        case eRampTypeEaseOut:
            return rampFunc<eRampTypeEaseOut>(t);
            break;
        case eRampTypeSmooth:
            return rampFunc<eRampTypeSmooth>(t);
            break;
        case eRampTypeNone:
            t = 1.;
            break;
        default:
            break;
    }
}
