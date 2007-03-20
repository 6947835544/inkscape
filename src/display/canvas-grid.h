#ifndef INKSCAPE_CANVAS_GRID_H
#define INKSCAPE_CANVAS_GRID_H

/*
 * Inkscape::CXYGrid
 *
 * Generic (and quite unintelligent) grid item for gnome canvas
 *
 * Copyright (C) Johan Engelen 2006 <johan@shouraizou.nl>
 * Copyright (C) Lauris Kaplinski 2000
 *
 */

#include <display/sp-canvas.h>
#include "xml/repr.h"
#include <gtkmm/box.h>


#include <gtkmm.h>
#include "ui/widget/color-picker.h"
#include "ui/widget/scalar-unit.h"

#include "ui/widget/registered-widget.h"
#include "ui/widget/registry.h"
#include "ui/widget/tolerance-slider.h"

#include "xml/node-event-vector.h"

#include "snapper.h"
#include "line-snapper.h"

struct SPDesktop;
struct SPNamedView;

namespace Inkscape {

#define INKSCAPE_TYPE_GRID_CANVASITEM            (Inkscape::grid_canvasitem_get_type ())
#define INKSCAPE_GRID_CANVASITEM(obj)            (GTK_CHECK_CAST ((obj), INKSCAPE_TYPE_GRID_CANVASITEM, GridCanvasItem))
#define INKSCAPE_GRID_CANVASITEM_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), INKSCAPE_TYPE_GRID_CANVASITEM, GridCanvasItem))
#define INKSCAPE_IS_GRID_CANVASITEM(obj)         (GTK_CHECK_TYPE ((obj), INKSCAPE_TYPE_GRID_CANVASITEM))
#define INKSCAPE_IS_GRID_CANVASITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), INKSCAPE_TYPE_GRID_CANVASITEM))

class CanvasGrid;

/** \brief  All the variables that are tracked for a grid specific
            canvas item. */
struct GridCanvasItem : public SPCanvasItem{
    CanvasGrid *grid; // the owning grid object
};

struct GridCanvasItemClass {
    SPCanvasItemClass parent_class;
};

/* Standard Gtk function */
GtkType grid_canvasitem_get_type (void);



class CanvasGrid {
public:
    CanvasGrid(SPDesktop *desktop, Inkscape::XML::Node * in_repr);
    virtual ~CanvasGrid();
    
    static CanvasGrid* NewGrid(SPDesktop *desktop, Inkscape::XML::Node * in_repr, const char * gridtype);
    static void writeNewGridToRepr(Inkscape::XML::Node * repr, const char * gridtype);
    
    virtual void Update (NR::Matrix const &affine, unsigned int flags) = 0;
    virtual void Render (SPCanvasBuf *buf) = 0;
    
    virtual void readRepr() {};
    virtual void onReprAttrChanged (Inkscape::XML::Node * repr, const gchar *key, const gchar *oldval, const gchar *newval, bool is_interactive) {};
    
    virtual Gtk::Widget & getWidget() = 0;

    bool enabled;
    bool visible;
    void hide();
    void show();

    Inkscape::XML::Node * repr;
    
    Inkscape::Snapper* snapper;

    static void on_repr_attr_changed (Inkscape::XML::Node * repr, const gchar *key, const gchar *oldval, const gchar *newval, bool is_interactive, void * data);
protected:
    GridCanvasItem * canvasitem;

    SPNamedView * namedview;
    
    Gtk::VBox vbox;

private:
    CanvasGrid(const CanvasGrid&);
    CanvasGrid& operator=(const CanvasGrid&);
 
};


class CanvasXYGrid : public CanvasGrid {
public:
    CanvasXYGrid(SPDesktop *desktop, Inkscape::XML::Node * in_repr);
    ~CanvasXYGrid();

    void Update (NR::Matrix const &affine, unsigned int flags);
    void Render (SPCanvasBuf *buf);
    
    void readRepr();
    void onReprAttrChanged (Inkscape::XML::Node * repr, const gchar *key, const gchar *oldval, const gchar *newval, bool is_interactive);
    
    Gtk::Widget & getWidget();

    NR::Point origin;
    guint32 color;
    guint32 empcolor;
    gint  empspacing;
    SPUnit const* gridunit;

    NR::Point spacing; /**< Spacing between elements of the grid */
    bool scaled[2];    /**< Whether the grid is in scaled mode, which can
                            be different in the X or Y direction, hense two
                            variables */
    NR::Point ow;      /**< Transformed origin by the affine for the zoom */
    NR::Point sw;      /**< Transformed spacing by the affine for the zoom */
private:
    CanvasXYGrid(const CanvasXYGrid&);
    CanvasXYGrid& operator=(const CanvasXYGrid&);
    
    void updateWidgets();

    Gtk::Table table;
    
    Inkscape::UI::Widget::RegisteredCheckButton _rcbgrid, _rcbsnbb, _rcbsnnod;
    Inkscape::UI::Widget::RegisteredRadioButtonPair _rrb_gridtype;
    Inkscape::UI::Widget::RegisteredUnitMenu    _rumg, _rums;
    Inkscape::UI::Widget::RegisteredScalarUnit  _rsu_ox, _rsu_oy, _rsu_sx, _rsu_sy, _rsu_ax, _rsu_az;
    Inkscape::UI::Widget::RegisteredColorPicker _rcp_gcol, _rcp_gmcol;
    Inkscape::UI::Widget::RegisteredSuffixedInteger _rsi;
    
    Inkscape::UI::Widget::Registry _wr;

};



class CanvasXYGridSnapper : public LineSnapper
{
public:
    CanvasXYGridSnapper(CanvasXYGrid *grid, SPNamedView const *nv, NR::Coord const d);

private:    
    LineList _getSnapLines(NR::Point const &p) const;
    
    CanvasXYGrid *grid;
}; 



















#define INKSCAPE_TYPE_CXYGRID            (Inkscape::cxygrid_get_type ())
#define INKSCAPE_CXYGRID(obj)            (GTK_CHECK_CAST ((obj), INKSCAPE_TYPE_CXYGRID, CXYGrid))
#define INKSCAPE_CXYGRID_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), INKSCAPE_TYPE_CXYGRID, CXYGridClass))
#define INKSCAPE_IS_CXYGRID(obj)         (GTK_CHECK_TYPE ((obj), INKSCAPE_TYPE_CXYGRID))
#define INKSCAPE_IS_CXYGRID_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), INKSCAPE_TYPE_CXYGRID))


/** \brief  All the variables that are tracked for a grid specific
            canvas item. */
struct CXYGrid : public SPCanvasItem{
	NR::Point origin;  /**< Origin of the grid */
	NR::Point spacing; /**< Spacing between elements of the grid */
	guint32 color;     /**< Color for normal lines */
	guint32 empcolor;  /**< Color for emphisis lines */
	gint empspacing;   /**< Spacing between emphisis lines */
	bool scaled[2];    /**< Whether the grid is in scaled mode, which can
					        be different in the X or Y direction, hense two
						    variables */
	NR::Point ow;      /**< Transformed origin by the affine for the zoom */
	NR::Point sw;      /**< Transformed spacing by the affine for the zoom */
};

struct CXYGridClass {
	SPCanvasItemClass parent_class;
};

/* Standard Gtk function */
GtkType cxygrid_get_type (void);

}; /* namespace Inkscape */




#endif
