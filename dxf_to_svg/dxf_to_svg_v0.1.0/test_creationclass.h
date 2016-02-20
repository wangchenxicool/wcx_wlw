/**
 * @file test_creationclass.h
 */

/*****************************************************************************
**  $Id: test_creationclass.h 8865 2008-02-04 18:54:02Z andrew $
**
**  This is part of the dxflib library
**  Copyright (C) 2001 Andrew Mustun
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU Library General Public License as
**  published by the Free Software Foundation.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU Library General Public License for more details.
**
**  You should have received a copy of the GNU Library General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
******************************************************************************/

#ifndef TEST_CREATIONCLASS_H
#define TEST_CREATIONCLASS_H

#define PI 3.14159265

#include "dl_creationadapter.h"
#include "mxml.h"


/**
 * This class takes care of the entities read from the file.
 * Usually such a class would probably store the entities.
 * this one just prints some information about them to stdout.
 *
 * @author Andrew Mustun
 */
class Test_CreationClass : public DL_CreationAdapter
{
public:
    Test_CreationClass();
    ~Test_CreationClass();
    mxml_node_t *xml;
    mxml_node_t *svg;
    mxml_node_t *g;
    FILE *fp;

    int X0;
    int Y0;

    void open_svg_file (const char *file);

    virtual void addLayer (const DL_LayerData& data);
    virtual void addPoint (const DL_PointData& data);
    virtual void addLine (const DL_LineData& data);
    virtual void addArc (const DL_ArcData& data);
    virtual void addCircle (const DL_CircleData& data);
    virtual void addPolyline (const DL_PolylineData& data);
    virtual void addVertex (const DL_VertexData& data);
    virtual void add3dFace (const DL_3dFaceData& data);

    virtual void addText (const DL_TextData& data);
    virtual void addMText (const DL_MTextData& data);
    virtual void addEllipse (const DL_EllipseData& data);

    void get_attributes (int *width, unsigned char *color_r, unsigned char *color_g, unsigned char *color_b);
};

#endif
