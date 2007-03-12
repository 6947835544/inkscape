/*
 * A simple utility for exporting Inkscape svg Shapes as PovRay bezier
 * prisms.  Note that this is output-only, and would thus seem to be
 * better placed as an 'export' rather than 'output'.  However, Export
 * handles all or partial documents, while this outputs ALL shapes in
 * the current SVG document.
 *
 *  For information on the PovRay file format, see:
 *      http://www.povray.org
 *
 * Authors:
 *   Bob Jamison <ishmalius@gmail.com>
 *
 * Copyright (C) 2004-2007 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "pov-out.h"
#include "inkscape.h"
#include "sp-path.h"
#include <style.h>
#include "display/curve.h"
#include "libnr/n-art-bpath.h"
#include "extension/system.h"

#include "io/sys.h"

#include <string>
#include <stdio.h>
#include <stdarg.h>


namespace Inkscape
{
namespace Extension
{
namespace Internal
{




//########################################################################
//# U T I L I T Y
//########################################################################



/**
 * This function searches the Repr tree recursively from the given node,
 * and adds refs to all nodes with the given name, to the result vector
 */
static void
findElementsByTagName(std::vector<Inkscape::XML::Node *> &results,
                      Inkscape::XML::Node *node,
                      char const *name)
{
    if ( !name || strcmp(node->name(), name) == 0 )
        results.push_back(node);

    for (Inkscape::XML::Node *child = node->firstChild() ; child ;
              child = child->next())
        findElementsByTagName( results, child, name );

}





static double
effective_opacity(SPItem const *item)
{
    double ret = 1.0;
    for (SPObject const *obj = item; obj; obj = obj->parent)
        {
        SPStyle const *const style = SP_OBJECT_STYLE(obj);
        g_return_val_if_fail(style, ret);
        ret *= SP_SCALE24_TO_FLOAT(style->opacity.value);
        }
    return ret;
}





//########################################################################
//# OUTPUT FORMATTING
//########################################################################

static const char *formatDouble(gchar *sbuffer, double d)
{
    return (const char *)g_ascii_formatd(sbuffer,
	         G_ASCII_DTOSTR_BUF_SIZE, "%.8g", (gdouble)d);

}


/**
 * Not-threadsafe version
 */
static char _dstr_buf[G_ASCII_DTOSTR_BUF_SIZE+1];

static const char *dstr(double d)
{
    return formatDouble(_dstr_buf, d);
}






/**
 *  Output data to the buffer, printf()-style
 */
void PovOutput::out(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    g_vsnprintf(fmtbuf, 4096, fmt, args);
    va_end(args);
    outbuf.append(fmtbuf);
}







/**
 *  Output a 3d vector
 */
void PovOutput::vec2(double a, double b)
{
    outbuf.append("<");
    outbuf.append(dstr(a));
    outbuf.append(", ");
    outbuf.append(dstr(b));
    outbuf.append(">");
}



/**
 * Output a 3d vector
 */
void PovOutput::vec3(double a, double b, double c)
{
    outbuf.append("<");
    outbuf.append(dstr(a));
    outbuf.append(", ");
    outbuf.append(dstr(b));
    outbuf.append(", ");
    outbuf.append(dstr(c));
    outbuf.append(">");
}



/**
 *  Output a v4d ector
 */
void PovOutput::vec4(double a, double b, double c, double d)
{
    outbuf.append("<");
    outbuf.append(dstr(a));
    outbuf.append(", ");
    outbuf.append(dstr(b));
    outbuf.append(", ");
    outbuf.append(dstr(c));
    outbuf.append(", ");
    outbuf.append(dstr(d));
    outbuf.append(">");
}


/**
 *  Output an rgbf color vector
 */
void PovOutput::rgbf(double r, double g, double b, double f)
{
    //"rgbf < %1.3f, %1.3f, %1.3f %1.3f>"
    outbuf.append("rgbf ");
    vec4(r, g, b, f);
}



/**
 *  Output one bezier's start, start-control, end-control, and end nodes
 */
void PovOutput::segment(int segNr, double a0, double a1,
                            double b0, double b1,
                            double c0, double c1,
                            double d0, double d1)
{
    //"    /*%4d*/ <%f, %f>, <%f, %f>, <%f,%f>, <%f,%f>"
    char buf[32];
    snprintf(buf, 31, "    /*%4d*/ ", segNr);
    outbuf.append(buf);
    vec2(a0, a1);
    outbuf.append(", ");
    vec2(b0, b1);
    outbuf.append(", ");
    vec2(c0, c1);
    outbuf.append(", ");
    vec2(d0, d1);
}





/**
 * Output the file header
 */
void PovOutput::doHeader()
{
    time_t tim = time(NULL);
    out("/*###################################################################\n");
    out("### This PovRay document was generated by Inkscape\n");
    out("### http://www.inkscape.org\n");
    out("### Created: %s", ctime(&tim));
    out("### Version: %s\n", VERSION);
    out("#####################################################################\n");
    out("### NOTES:\n");
    out("### ============\n");
    out("### POVRay information can be found at\n");
    out("### http://www.povray.org\n");
    out("###\n");
    out("### The 'AllShapes' objects at the bottom are provided as a\n");
    out("### preview of how the output would look in a trace.  However,\n");
    out("### the main intent of this file is to provide the individual\n");
    out("### shapes for inclusion in a POV project.\n");
    out("###\n");
    out("### For an example of how to use this file, look at\n");
    out("### share/examples/istest.pov\n");
    out("###################################################################*/\n");
    out("\n\n");
    out("/*###################################################################\n");
    out("##   Exports in this file\n");
    out("##==========================\n");
    out("##    Shapes   : %d\n", nrShapes);
    out("##    Segments : %d\n", nrSegments);
    out("##    Nodes    : %d\n", nrNodes);
    out("###################################################################*/\n");
    out("\n\n\n");
}



/**
 *  Output the file footer
 */
void PovOutput::doTail()
{
    out("\n\n");
    out("/*###################################################################\n");
    out("### E N D    F I L E\n");
    out("###################################################################*/\n");
    out("\n\n");
}



/**
 *  Output the curve data to buffer
 */
void PovOutput::doCurves(SPDocument *doc)
{
    std::vector<Inkscape::XML::Node *>results;
    //findElementsByTagName(results, SP_ACTIVE_DOCUMENT->rroot, "path");
    findElementsByTagName(results, SP_ACTIVE_DOCUMENT->rroot, NULL);
    if (results.size() == 0)
        return;

    double bignum = 1000000.0;
    double minx  =  bignum;
    double maxx  = -bignum;
    double miny  =  bignum;
    double maxy  = -bignum;

    for (unsigned int indx = 0; indx < results.size() ; indx++)
        {
        //### Fetch the object from the repr info
        Inkscape::XML::Node *rpath = results[indx];
        char *str  = (char *) rpath->attribute("id");
        if (!str)
            continue;

        String id = str;
        SPObject *reprobj = SP_ACTIVE_DOCUMENT->getObjectByRepr(rpath);
        if (!reprobj)
            continue;

        //### Get the transform of the item
        if (!SP_IS_ITEM(reprobj))
            continue;

        SPItem *item = SP_ITEM(reprobj);
        NR::Matrix tf = sp_item_i2d_affine(item);

        //### Get the Shape
        if (!SP_IS_SHAPE(reprobj))//Bulia's suggestion.  Allow all shapes
            continue;

        SPShape *shape = SP_SHAPE(reprobj);
        SPCurve *curve = shape->curve;
        if (sp_curve_empty(curve))
            continue;
            
        nrShapes++;

        PovShapeInfo shapeInfo;
        shapeInfo.id    = id;
        shapeInfo.color = "";

        //Try to get the fill color of the shape
        SPStyle *style = SP_OBJECT_STYLE(shape);
        /* fixme: Handle other fill types, even if this means translating gradients to a single
           flat colour. */
        if (style && (style->fill.type == SP_PAINT_TYPE_COLOR))
            {
            // see color.h for how to parse SPColor
            float rgb[3];
            sp_color_get_rgb_floatv(&style->fill.value.color, rgb);
            double const dopacity = ( SP_SCALE24_TO_FLOAT(style->fill_opacity.value)
                                      * effective_opacity(shape) );
            //gchar *str = g_strdup_printf("rgbf < %1.3f, %1.3f, %1.3f %1.3f>",
            //                             rgb[0], rgb[1], rgb[2], 1.0 - dopacity);
            String rgbf = "rgbf <";
            rgbf.append(dstr(rgb[0]));         rgbf.append(", ");
            rgbf.append(dstr(rgb[1]));         rgbf.append(", ");
            rgbf.append(dstr(rgb[2]));         rgbf.append(", ");
            rgbf.append(dstr(1.0 - dopacity)); rgbf.append(">");
            shapeInfo.color += rgbf;
            }

        povShapes.push_back(shapeInfo); //passed all tests.  save the info

        int curveLength = SP_CURVE_LENGTH(curve);

        //Count the NR_CURVETOs/LINETOs
        int segmentCount=0;
        NArtBpath *bp = SP_CURVE_BPATH(curve);
        for (int curveNr=0 ; curveNr<curveLength ; curveNr++, bp++)
            if (bp->code == NR_CURVETO || bp->code == NR_LINETO)
                segmentCount++;

        double cminx  =  bignum;
        double cmaxx  = -bignum;
        double cminy  =  bignum;
        double cmaxy  = -bignum;
        double lastx  = 0.0;
        double lasty  = 0.0;

        out("/*###################################################\n");
        out("### PRISM:  %s\n", id.c_str());
        out("###################################################*/\n");
        out("#declare %s = prism {\n", id.c_str());
        out("    linear_sweep\n");
        out("    bezier_spline\n");
        out("    1.0, //top\n");
        out("    0.0, //bottom\n");
        out("    %d //nr points\n", segmentCount * 4);
        int segmentNr = 0;
        bp = SP_CURVE_BPATH(curve);
        
        nrSegments += curveLength;

        for (int curveNr=0 ; curveNr < curveLength ; curveNr++)
            {
            using NR::X;
            using NR::Y;
            NR::Point const p1(bp->c(1) * tf);
            NR::Point const p2(bp->c(2) * tf);
            NR::Point const p3(bp->c(3) * tf);
            double const x1 = p1[X], y1 = p1[Y];
            double const x2 = p2[X], y2 = p2[Y];
            double const x3 = p3[X], y3 = p3[Y];

            switch (bp->code)
                {
                case NR_MOVETO:
                case NR_MOVETO_OPEN:
                    {
                    //fprintf(f, "moveto: %f %f\n", bp->x3, bp->y3);
                    break;
                    }
                case NR_CURVETO:
                    {
                    //fprintf(f, "    /*%4d*/ <%f, %f>, <%f, %f>, <%f,%f>, <%f,%f>",
                    //        segmentNr++, lastx, lasty, x1, y1, x2, y2, x3, y3);
                    segment(segmentNr++,
                          lastx, lasty, x1, y1, x2, y2, x3, y3);
                    nrNodes += 8;

                    if (segmentNr < segmentCount)
                        out(",\n");
                    else
                        out("\n");

                    if (lastx < cminx)
                        cminx = lastx;
                    if (lastx > cmaxx)
                        cmaxx = lastx;
                    if (lasty < cminy)
                        cminy = lasty;
                    if (lasty > cmaxy)
                        cmaxy = lasty;
                    break;
                    }
                case NR_LINETO:
                    {
                    //fprintf(f, "    /*%4d*/ <%f, %f>, <%f, %f>, <%f,%f>, <%f,%f>",
                    //        segmentNr++, lastx, lasty, lastx, lasty, x3, y3, x3, y3);
                    segment(segmentNr++,
                         lastx, lasty, lastx, lasty, x3, y3, x3, y3);
                    nrNodes += 8;

                    if (segmentNr < segmentCount)
                        out(",\n");
                    else
                        out("\n");

                    //fprintf(f, "lineto\n");
                    if (lastx < cminx)
                        cminx = lastx;
                    if (lastx > cmaxx)
                        cmaxx = lastx;
                    if (lasty < cminy)
                        cminy = lasty;
                    if (lasty > cmaxy)
                        cmaxy = lasty;
                    break;
                    }
                case NR_END:
                    {
                    //fprintf(f, "end\n");
                    break;
                    }
                }
            lastx = x3;
            lasty = y3;
            bp++;
            }
        out("}\n");


	    char *pfx = (char *)id.c_str();

        out("#declare %s_MIN_X    = %s;\n", pfx, dstr(cminx));
        out("#declare %s_CENTER_X = %s;\n", pfx, dstr((cmaxx+cminx)/2.0));
        out("#declare %s_MAX_X    = %s;\n", pfx, dstr(cmaxx));
        out("#declare %s_WIDTH    = %s;\n", pfx, dstr(cmaxx-cminx));
        out("#declare %s_MIN_Y    = %s;\n", pfx, dstr(cminy));
        out("#declare %s_CENTER_Y = %s;\n", pfx, dstr((cmaxy+cminy)/2.0));
        out("#declare %s_MAX_Y    = %s;\n", pfx, dstr(cmaxy));
        out("#declare %s_HEIGHT   = %s;\n", pfx, dstr(cmaxy-cminy));
        if (shapeInfo.color.length()>0)
            out("#declare %s_COLOR    = %s;\n",
                    pfx, shapeInfo.color.c_str());
        out("/*###################################################\n");
        out("### end %s\n", id.c_str());
        out("###################################################*/\n\n\n\n");
        if (cminx < minx)
            minx = cminx;
        if (cmaxx > maxx)
            maxx = cmaxx;
        if (cminy < miny)
            miny = cminy;
        if (cmaxy > maxy)
            maxy = cmaxy;

        }//for



    //## Let's make a union of all of the Shapes
    if (povShapes.size()>0)
        {
        String id = "AllShapes";
        char *pfx = (char *)id.c_str();
        out("/*###################################################\n");
        out("### UNION OF ALL SHAPES IN DOCUMENT\n");
        out("###################################################*/\n");
        out("\n\n");
        out("/**\n");
        out(" * Allow the user to redefine the finish{}\n");
        out(" * by declaring it before #including this file\n");
        out(" */\n");
        out("#ifndef (%s_Finish)\n", pfx);
        out("#declare %s_Finish = finish {\n", pfx);
        out("    phong 0.5\n");
        out("    reflection 0.3\n");
        out("    specular 0.5\n");
        out("}\n");
        out("#end\n");
        out("\n\n");
        out("#declare %s = union {\n", id.c_str());
        for (unsigned i = 0 ; i < povShapes.size() ; i++)
            {
            out("    object { %s\n", povShapes[i].id.c_str());
            out("        texture { \n");
            if (povShapes[i].color.length()>0)
                out("            pigment { %s }\n", povShapes[i].color.c_str());
            else
                out("            pigment { rgb <0,0,0> }\n");
            out("            finish { %s_Finish }\n", pfx);
            out("            } \n");
            out("        } \n");
            }
        out("}\n\n\n\n");


        double zinc   = 0.2 / (double)povShapes.size();
        out("/*#### Same union, but with Z-diffs (actually Y in pov) ####*/\n");
        out("\n\n");
        out("/**\n");
        out(" * Allow the user to redefine the Z-Increment\n");
        out(" */\n");
        out("#ifndef (AllShapes_Z_Increment)\n");
        out("#declare AllShapes_Z_Increment = %s;\n", dstr(zinc));
        out("#end\n");
        out("\n");
        out("#declare AllShapes_Z_Scale = 1.0;\n");
        out("\n\n");
        out("#declare %s_Z = union {\n", pfx);

        for (unsigned i = 0 ; i < povShapes.size() ; i++)
            {
            out("    object { %s\n", povShapes[i].id.c_str());
            out("        texture { \n");
            if (povShapes[i].color.length()>0)
                out("            pigment { %s }\n", povShapes[i].color.c_str());
            else
                out("            pigment { rgb <0,0,0> }\n");
            out("            finish { %s_Finish }\n", pfx);
            out("            } \n");
            out("        scale <1, %s_Z_Scale, 1>\n", pfx);
            out("        } \n");
            out("#declare %s_Z_Scale = %s_Z_Scale + %s_Z_Increment;\n\n",
                    pfx, pfx, pfx);
            }

        out("}\n");

        out("#declare %s_MIN_X    = %s;\n", pfx, dstr(minx));
        out("#declare %s_CENTER_X = %s;\n", pfx, dstr((maxx+minx)/2.0));
        out("#declare %s_MAX_X    = %s;\n", pfx, dstr(maxx));
        out("#declare %s_WIDTH    = %s;\n", pfx, dstr(maxx-minx));
        out("#declare %s_MIN_Y    = %s;\n", pfx, dstr(miny));
        out("#declare %s_CENTER_Y = %s;\n", pfx, dstr((maxy+miny)/2.0));
        out("#declare %s_MAX_Y    = %s;\n", pfx, dstr(maxy));
        out("#declare %s_HEIGHT   = %s;\n", pfx, dstr(maxy-miny));
        out("/*##############################################\n");
        out("### end %s\n", id.c_str());
        out("##############################################*/\n");
        out("\n\n");
        }

}




//########################################################################
//# M A I N    O U T P U T
//########################################################################



/**
 *  Set values back to initial state
 */
void PovOutput::reset()
{
    nrNodes    = 0;
    nrSegments = 0;
    nrShapes   = 0;
    outbuf.clear();
    povShapes.clear();
}



/**
 * Saves the <paths> of an Inkscape SVG file as PovRay spline definitions
 */
void PovOutput::saveDocument(SPDocument *doc, gchar const *uri)
{
    reset();

    //###### SAVE IN POV FORMAT TO BUFFER
    //# Lets do the curves first, to get the stats
    doCurves(doc);
    String curveBuf = outbuf;
    outbuf.clear();

    doHeader();
    
    outbuf.append(curveBuf);
    
    doTail();




    //###### WRITE TO FILE
    Inkscape::IO::dump_fopen_call(uri, "L");
    FILE *f = Inkscape::IO::fopen_utf8name(uri, "w");
    if (!f)
        return;

    for (String::iterator iter = outbuf.begin() ; iter!=outbuf.end(); iter++)
        {
        int ch = *iter;
        fputc(ch, f);
        }
        
    fclose(f);
}




//########################################################################
//# EXTENSION API
//########################################################################



#include "clear-n_.h"



/**
 * API call to save document
*/
void
PovOutput::save(Inkscape::Extension::Output *mod,
                        SPDocument *doc, gchar const *uri)
{
    saveDocument(doc, uri);
}



/**
 * Make sure that we are in the database
 */
bool PovOutput::check (Inkscape::Extension::Extension *module)
{
    /* We don't need a Key
    if (NULL == Inkscape::Extension::db.get(SP_MODULE_KEY_OUTPUT_POV))
        return FALSE;
    */

    return true;
}



/**
 * This is the definition of PovRay output.  This function just
 * calls the extension system with the memory allocated XML that
 * describes the data.
*/
void
PovOutput::init()
{
    Inkscape::Extension::build_from_mem(
        "<inkscape-extension>\n"
            "<name>" N_("PovRay Output") "</name>\n"
            "<id>org.inkscape.output.pov</id>\n"
            "<output>\n"
                "<extension>.pov</extension>\n"
                "<mimetype>text/x-povray-script</mimetype>\n"
                "<filetypename>" N_("PovRay (*.pov) (export splines)") "</filetypename>\n"
                "<filetypetooltip>" N_("PovRay Raytracer File") "</filetypetooltip>\n"
            "</output>\n"
        "</inkscape-extension>",
        new PovOutput());
}





}  // namespace Internal
}  // namespace Extension
}  // namespace Inkscape


/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99 :
