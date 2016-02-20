/*
 * @file test_creationclass.cpp
 */

/*****************************************************************************
**  $Id: test_creationclass.cpp 8865 2008-02-04 18:54:02Z andrew $
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
#include <iostream>
#include <stdio.h>
#ifndef WIN32
#include <unistd.h>
#endif /* !WIN32 */
#include <fcntl.h>
#ifndef O_BINARY
#define O_BINARY 0
#endif /* !O_BINARY */
#include "test_creationclass.h"
#include "mxml.h"


#ifdef DEBUG
#define wprintf(format, arg...)    \
    printf (format , ##arg)
#else
#define wprintf(format, arg...) {}
#endif

unsigned char color_value[][3] =
{
    {0, 0, 0},
    {255, 0, 0},
    {255, 255, 0},
    {0, 255, 0},
    {0, 255, 255},
    {0, 0, 255},
    {255, 0, 255},
    {255, 255, 255},
    {128, 128, 128},
    {192, 192, 192},
    {255, 0, 0},
    {255, 120, 112},
    {192, 0, 0},
    {207, 103, 96},
    {144, 0, 0},
    {144, 72, 79},
    {127, 0, 0},
    {112, 56, 48},
    {79, 0, 0},
    {79, 39, 32},
    {255, 56, 0},
    {255, 159, 127},
    {192, 48, 0},
    {207, 127, 96},
    {144, 32, 0},
    {144, 88, 64},
    {112, 24, 0},
    {127, 72, 63},
    {79, 16, 0},
    {64, 40, 31},
    {255, 127, 0},
    {255, 191, 127},
    {207, 103, 0},
    {192, 151, 96},
    {159, 72, 0},
    {144, 112, 79},
    {127, 63, 0},
    {127, 95, 63},
    {64, 32, 0},
    {64, 55, 31},
    {255, 191, 0},
    {255, 216, 127},
    {207, 152, 0},
    {192, 175, 96},
    {144, 112, 0},
    {144, 128, 64},
    {112, 88, 0},
    {127, 111, 63},
    {64, 55, 0},
    {64, 63, 31},
    {255, 255, 0},
    {255, 255, 112},
    {207, 200, 0},
    {192, 200, 96},
    {144, 151, 0},
    {144, 151, 64},
    {144, 152, 0},
    {127, 120, 63},
    {79, 72, 0},
    {79, 72, 32},
    {191, 255, 0},
    {208, 255, 112},
    {144, 200, 0},
    {176, 200, 96},
    {111, 151, 0},
    {128, 151, 79},
    {95, 127, 0},
    {111, 127, 63},
    {48, 72, 0},
    {63, 72, 31},
    {127, 255, 0},
    {191, 255, 127},
    {95, 200, 0},
    {159, 200, 96},
    {64, 151, 0},
    {111, 151, 64},
    {63, 120, 0},
    {95, 127, 63},
    {31, 72, 0},
    {48, 72, 32},
    {63, 255, 0},
    {159, 255, 127},
    {47, 200, 0},
    {127, 200, 96},
    {31, 151, 0},
    {80, 151, 64},
    {31, 127, 0},
    {79, 127, 63},
    {15, 72, 0},
    {47, 72, 32},
    {0, 255, 0},
    {127, 255, 127},
    {0, 200, 0},
    {95, 200, 95},
    {0, 151, 0},
    {79, 151, 79},
    {0, 127, 0},
    {63, 127, 63},
    {0, 72, 0},
    {32, 72, 32},
    {0, 255, 63},
    {127, 255, 159},
    {0, 200, 47},
    {95, 200, 112},
    {0, 151, 32},
    {64, 151, 95},
    {0, 120, 31},
    {63, 127, 79},
    {0, 72, 15},
    {32, 72, 47},
    {0, 255, 127},
    {127, 255, 191},
    {0, 200, 96},
    {95, 200, 144},
    {0, 151, 64},
    {79, 151, 111},
    {0, 127, 63},
    {63, 127, 95},
    {0, 72, 32},
    {31, 72, 48},
    {0, 255, 191},
    {127, 255, 223},
    {0, 200, 144},
    {95, 200, 175},
    {0, 151, 111},
    {79, 151, 128},
    {0, 120, 95},
    {63, 127, 111},
    {0, 72, 48},
    {32, 72, 63},
    {0, 255, 255},
    {127, 255, 255},
    {0, 200, 207},
    {96, 200, 207},
    {0, 151, 144},
    {64, 151, 144},
    {0, 127, 127},
    {63, 127, 127},
    {0, 72, 79},
    {32, 72, 79},
    {0, 191, 255},
    {127, 223, 255},
    {0, 152, 207},
    {96, 176, 207},
    {0, 111, 144},
    {64, 128, 144},
    {0, 95, 127},
    {63, 111, 127},
    {0, 55, 64},
    {31, 63, 64},
    {0, 127, 255},
    {112, 184, 255},
    {0, 96, 192},
    {96, 151, 207},
    {0, 72, 159},
    {79, 112, 144},
    {0, 63, 127},
    {63, 95, 127},
    {0, 39, 79},
    {32, 56, 79},
    {0, 63, 255},
    {127, 159, 255},
    {0, 48, 207},
    {96, 127, 207},
    {0, 39, 159},
    {64, 95, 144},
    {0, 24, 127},
    {63, 79, 127},
    {0, 15, 64},
    {32, 47, 79},
    {0, 0, 255},
    {127, 127, 255},
    {0, 0, 207},
    {95, 96, 192},
    {0, 0, 144},
    {79, 72, 159},
    {0, 0, 127},
    {63, 63, 127},
    {0, 0, 64},
    {32, 39, 79},
    {63, 0, 255},
    {159, 127, 255},
    {47, 0, 192},
    {127, 103, 207},
    {32, 0, 144},
    {80, 72, 144},
    {31, 0, 127},
    {79, 63, 127},
    {15, 0, 79},
    {47, 39, 79},
    {127, 0, 255},
    {191, 127, 255},
    {96, 0, 207},
    {144, 103, 207},
    {79, 0, 144},
    {111, 72, 144},
    {63, 0, 127},
    {95, 63, 127},
    {31, 0, 64},
    {48, 39, 79},
    {191, 0, 255},
    {223, 127, 255},
    {144, 0, 207},
    {175, 96, 192},
    {111, 0, 144},
    {128, 72, 159},
    {95, 0, 127},
    {96, 56, 112},
    {48, 0, 64},
    {64, 39, 79},
    {255, 0, 255},
    {255, 127, 255},
    {192, 0, 192},
    {192, 96, 192},
    {144, 0, 144},
    {144, 72, 144},
    {127, 0, 127},
    {127, 63, 127},
    {79, 0, 79},
    {64, 32, 64},
    {255, 0, 191},
    {255, 127, 223},
    {192, 0, 144},
    {192, 96, 175},
    {144, 0, 111},
    {144, 72, 128},
    {127, 0, 95},
    {127, 56, 111},
    {64, 0, 48},
    {79, 39, 63},
    {255, 0, 127},
    {255, 127, 191},
    {192, 0, 95},
    {192, 96, 144},
    {159, 0, 79},
    {144, 72, 111},
    {127, 0, 63},
    {127, 63, 95},
    {64, 0, 31},
    {64, 32, 48},
    {255, 0, 63},
    {255, 127, 159},
    {207, 0, 48},
    {207, 103, 127},
    {144, 0, 32},
    {144, 72, 95},
    {127, 0, 31},
    {127, 63, 79},
    {79, 0, 15},
    {64, 32, 47},
    {47, 47, 47},
    {80, 88, 80},
    {128, 128, 128},
    {175, 175, 175},
    {208, 215, 208},
    {255, 255, 255}
};

/**
 * Default constructor.
 */
Test_CreationClass::Test_CreationClass()
{
    xml = mxmlNewXML ("1.0");
    svg = mxmlNewElement (xml, "svg");

    /* svg ×ø±ê±ä»» */
    //g = mxmlNewElement (svg, "g");
    //mxmlElementSetAttr (g, "transform", "translate(0,1200)");

    /* add background */
    mxml_node_t *rect;
    rect = mxmlNewElement (svg, "rect");
    mxmlElementSetAttr (rect, "x", "0");
    mxmlElementSetAttr (rect, "y", "0");
    mxmlElementSetAttr (rect, "width", "1800");
    mxmlElementSetAttr (rect, "height", "1200");
    mxmlElementSetAttr (rect, "style", "fill:rgb(0,0,0)");

}

Test_CreationClass::~Test_CreationClass()
{
    mxmlSaveFile (xml, fp, MXML_NO_CALLBACK);
    fclose (fp);
}

void Test_CreationClass::open_svg_file (const char *file)
{
    fp = fopen (file, "w");
}


/**
 * Sample implementation of the method which handles layers.
 */
void Test_CreationClass::addLayer (const DL_LayerData& data)
{
    /* get attributs */
    unsigned char color_r;
    unsigned char color_g;
    unsigned char color_b;
    int width;
    get_attributes (NULL, &width, &color_r, &color_g, &color_b);

    wprintf ("LAYER: %s flags: %d\n", data.name.c_str(), data.flags);
}

/**
 * Sample implementation of the method which handles point entities.
 */
void Test_CreationClass::addPoint (const DL_PointData& data)
{
    wprintf ("POINT    (%6.3f, %6.3f, %6.3f)\n",
             data.x, data.y, data.z);
}

/**
 * Sample implementation of the method which handles line entities.
 */
void Test_CreationClass::addLine (const DL_LineData& data)
{
    /* get attributs */
    unsigned char color_r;
    unsigned char color_g;
    unsigned char color_b;
    int width;
    get_attributes (NULL, &width, &color_r, &color_g, &color_b);

    /* write svg */
    mxml_node_t *line;
    char x1[50];
    char y1[50];
    char x2[50];
    char y2[50];

    sprintf (x1, "%6.3f", data.x1);
    sprintf (y1, "%6.3f", (data.y1));
    sprintf (x2, "%6.3f", data.x2);
    sprintf (y2, "%6.3f", (data.y2));

    printf ("##### x1:%s,y1:%s,x2:%s,y2:%s\n", x1, y1, x2, y2);

    line = mxmlNewElement (svg, "line");
    mxmlElementSetAttr (line, "x1", x1);
    mxmlElementSetAttr (line, "y1", y1);
    mxmlElementSetAttr (line, "x2", x2);
    mxmlElementSetAttr (line, "y2", y2);
    char style_value[128];
    sprintf (style_value, "stroke:rgb(%d,%d,%d);stroke-width:%d", color_r, color_g, color_b, width);
    mxmlElementSetAttr (line, "style", style_value);
}

/**
 * Sample implementation of the method which handles arc entities.
 */
void Test_CreationClass::addArc (const DL_ArcData& data)
{
    printf ("ARC      (%6.3f, %6.3f, %6.3f) %6.3f, %6.3f, %6.3f\n",
            data.cx, data.cy, data.cz,
            data.radius, data.angle1, data.angle2);

    /* get attributs */
    unsigned char color_r;
    unsigned char color_g;
    unsigned char color_b;
    int width;
    get_attributes (NULL, &width, &color_r, &color_g, &color_b);

    mxml_node_t *arc;
    char cx[50];
    char cy[50];
    char r[50];
    char ang1[50];
    char ang2[50];

    sprintf (cx, "%6.3f", data.cx);
    sprintf (cy, "%6.3f", (data.cy));
    sprintf (r, "%6.3f", data.radius);
    sprintf (ang1, "%6.3f", data.angle1 * 2 * PI / 360);
    sprintf (ang2, "%6.3f", data.angle2 * 2 * PI / 360);

    arc = mxmlNewElement (svg, "arc");
    mxmlElementSetAttr (arc, "cx", cx);
    mxmlElementSetAttr (arc, "cy", cy);
    mxmlElementSetAttr (arc, "r", r);
    mxmlElementSetAttr (arc, "ang1", ang1);
    mxmlElementSetAttr (arc, "ang2", ang2);
    //char style_value[256];
    //sprintf (style_value, "stroke:rgb(%d,%d,%d);stroke-width:%d", color_r, color_g, color_b, width);
    //mxmlElementSetAttr (arc, "style", style_value);
    char value[256];
    sprintf (value, "rgb(%d,%d,%d)", color_r, color_g, color_b);
    mxmlElementSetAttr (arc, "stroke", value);
    sprintf (value, "%d", width);
    mxmlElementSetAttr (arc, "stroke-width", value);
}

/**
 * Sample implementation of the method which handles circle entities.
 */
void Test_CreationClass::addCircle (const DL_CircleData& data)
{
    /* get attributs */
    unsigned char color_r;
    unsigned char color_g;
    unsigned char color_b;
    int width;
    get_attributes (NULL, &width, &color_r, &color_g, &color_b);

    /* write svg */
    mxml_node_t *circle;
    char cx[50];
    char cy[50];
    char radius[50];

    sprintf (cx, "%6.3f", data.cx);
    sprintf (cy, "%6.3f", (data.cy));
    sprintf (radius, "%6.3f", data.radius);

    circle = mxmlNewElement (svg, "circle");
    mxmlElementSetAttr (circle, "cx", cx);
    mxmlElementSetAttr (circle, "cy", cy);
    mxmlElementSetAttr (circle, "r", radius);
    char value[50];
    sprintf (value, "rgb(%d,%d,%d)", color_r, color_g, color_b);
    mxmlElementSetAttr (circle, "stroke", value);
    sprintf (value, "%d", width);
    mxmlElementSetAttr (circle, "stroke-width", value);
    mxmlElementSetAttr (circle, "fill", "rgb(0,0,0)");
}


/**
 * Sample implementation of the method which handles polyline entities.
 */
void Test_CreationClass::addPolyline (const DL_PolylineData& data)
{
    unsigned char color_r;
    unsigned char color_g;
    unsigned char color_b;
    int width;

    /* get attributs */
    get_attributes (NULL, &width, &color_r, &color_g, &color_b);

    printf ("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!POLYLINE \n");
    printf ("flags: %d\n", (int) data.flags);
}


/**
 * Sample implementation of the method which handles vertices.
 */
void Test_CreationClass::addVertex (const DL_VertexData& data)
{
    unsigned char color_r;
    unsigned char color_g;
    unsigned char color_b;
    int width;

    /* get attributs */
    get_attributes (NULL, &width, &color_r, &color_g, &color_b);

    wprintf ("VERTEX   (%6.3f, %6.3f, %6.3f) %6.3f\n",
             data.x, data.y, data.z,
             data.bulge);
}


void Test_CreationClass::add3dFace (const DL_3dFaceData& data)
{
    unsigned char color_r;
    unsigned char color_g;
    unsigned char color_b;
    int width;

    /* get attributs */
    get_attributes (NULL, &width, &color_r, &color_g, &color_b);

    wprintf ("3DFACE\n");
    for (int i = 0; i < 4; i++)
    {
        wprintf ("   corner %d: %6.3f %6.3f %6.3f\n", i, data.x[i], data.y[i], data.z[i]);
    }
}

void Test_CreationClass::get_attributes (char *layer, int *width,
        unsigned char *color_r, unsigned char *color_g, unsigned char *color_b)
{
    *width = 1;
    *color_r = 99;
    *color_g = 99;
    *color_b = 99;

    /* get layer */
    printf ("  Attributes: Layer: %s, ", attributes.getLayer().c_str());
    if (layer != NULL)
    {
        sprintf (layer, "%s", attributes.getLayer().c_str());
    }

    /* get color */
    printf (" Color: ");
    if (attributes.getColor() == 256)
        printf ("BYLAYER");
    else if (attributes.getColor() == 0)
        printf ("BYBLOCK");
    else
    {
        int i = attributes.getColor();
        printf ("%d", i);
        *color_r = color_value[i][0];
        *color_g = color_value[i][1];
        *color_b = color_value[i][2];
    }

    /* get width */
    printf (" Width: ");
    if (attributes.getWidth() == -1)
        printf ("BYLAYER");
    else if (attributes.getWidth() == -2)
        printf ("BYBLOCK");
    else if (attributes.getWidth() == -3)
        printf ("DEFAULT");
    else
    {
        *width = attributes.getWidth() / 10;
        printf ("%d", *width);
    }

    /* get type */
    printf (" Type: %s\n", attributes.getLineType().c_str());
}

void Test_CreationClass::addText (const DL_TextData& data)
{
    printf ("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$@@@@@@@text:%s\n", data.text.c_str());
    if (strchr (data.text.c_str(), '¡ã'))
    {
        return;
    }

    /* get attributs */
    char layer[250];
    unsigned char color_r;
    unsigned char color_g;
    unsigned char color_b;
    int width;
    get_attributes (layer, &width, &color_r, &color_g, &color_b);

    /* write svg */
    mxml_node_t *text;
    char x[50];
    char y[50];

    sprintf (x, "%6.3f", data.ipx);
    sprintf (y, "%6.3f", (data.ipy));

    /* get text */
    char *cstr;
    cstr = new char [data.text.size() + 1];
    strcpy (cstr, data.text.c_str());

    printf ("~~~~~~~~~~~~~ the text layer: %s\n", layer);
    if (strstr (layer, "DTEXT") != 0) // is dtext
    {
        text = mxmlNewElement (svg, "dtext");
        mxmlElementSetAttr (text, "id", cstr);
    }
    else
    {
        text = mxmlNewElement (svg, "text");
    }
    mxmlElementSetAttr (text, "x", x);
    mxmlElementSetAttr (text, "y", y);

    char rotate[50];
    sprintf (rotate, "rotate(%6.3f,%s,%s)", data.angle, x, y);
    mxmlElementSetAttr (text, "transform", rotate);
    mxmlElementSetAttr (text, "font-family", "SimHei");
    mxmlElementSetAttr (text, "font-size", "18");

    char rgb_value[50];
    sprintf (rgb_value, "rgb(%d,%d,%d)", color_r, color_g, color_b);
    mxmlElementSetAttr (text, "fill", rgb_value);

    mxmlNewText (text, 1, cstr);
    delete[] cstr;
}

void Test_CreationClass::addMText (const DL_MTextData& data)
{
    printf ("@@@@@@@text:%s\n", data.text.c_str());
    if (strchr (data.text.c_str(), '¡ã'))
    {
        return;
    }

    /* get attributs */
    char layer[250];
    unsigned char color_r;
    unsigned char color_g;
    unsigned char color_b;
    int width;
    get_attributes (layer, &width, &color_r, &color_g, &color_b);

    /* write svg */
    mxml_node_t *text;
    char x[50];
    char y[50];

    /* get text */
    char *cstr;
    cstr = new char [data.text.size() + 1];
    strcpy (cstr, data.text.c_str());

    sprintf (x, "%6.3f", data.ipx);
    sprintf (y, "%6.3f", (data.ipy));

    if (strstr (layer, "DTEXT") != 0) // is dtext
    {
        text = mxmlNewElement (svg, "dtext");
        mxmlElementSetAttr (text, "id", cstr);
    }
    else
    {
        text = mxmlNewElement (svg, "text");
    }
    mxmlElementSetAttr (text, "x", x);
    mxmlElementSetAttr (text, "y", y);

    char rotate[50];
    sprintf (rotate, "rotate(%6.3f,0,0)", data.angle);
    mxmlElementSetAttr (text, "transform", rotate);
    mxmlElementSetAttr (text, "font-family", "SimHei");
    mxmlElementSetAttr (text, "font-size", "18");
    mxmlElementSetAttr (text, "fill", "rgb(0,255,0)");
    mxmlNewText (text, 1, cstr);
    delete[] cstr;
}

void Test_CreationClass::addEllipse (const DL_EllipseData& data)
{
    /* get attributs */
    unsigned char color_r;
    unsigned char color_g;
    unsigned char color_b;
    int width;
    get_attributes (NULL, &width, &color_r, &color_g, &color_b);

    /* write svg */
    mxml_node_t *ellipse;
    char cx[50];
    char cy[50];
    char rx[50];
    char ry[50];
    double drx = sqrt (data.mx * data.mx + data.my * data.my);
    double dry = drx * data.ratio;

    sprintf (cx, "%6.3f", data.cx);
    sprintf (cy, "%6.3f", (data.cy));
    sprintf (rx, "%6.3f", drx);
    sprintf (ry, "%6.3f", dry);

    //printf ("cx:%6.3f, cy:%6.3f, mx:%6.3f, my:%6.3f, ratio:%6.3f, angle1:%6.3f, angle2:%6.3f\n",
    //data.cx, data.cy, rx, ry, data.ratio, data.angle1, data.angle2);

    ellipse = mxmlNewElement (svg, "ellipse");
    mxmlElementSetAttr (ellipse, "cx", cx);
    mxmlElementSetAttr (ellipse, "cy", cy);
    mxmlElementSetAttr (ellipse, "rx", rx);
    mxmlElementSetAttr (ellipse, "ry", ry);
    char style_value[128];
    sprintf (style_value, "fill:rgb(0,0,0);stroke:rgb(%d,%d,%d);stroke-width:%d", color_r, color_g, color_b, width);
    mxmlElementSetAttr (ellipse, "style", style_value);

    char rotate[256];
    sprintf (rotate, "rotate(%3.3f,%s,%s)", -atan (data.my / data.mx) * 180 / 3.1415926, cx, cy);
    mxmlElementSetAttr (ellipse, "transform", rotate);
}
// EOF
