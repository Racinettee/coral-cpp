#ifndef _GTKPP_
#define _GTKPP_

#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

/// GtkPP is a looser wrapper than gtkmm. It does not attempt to manage the ref
/// count as does gtkmm as it does not assume that you want a widget to be
/// deleted when a handle to one of these wrapper objects goes out of scope.
namespace GtkPP
{
  class Widget
  {
  public:
    GtkWidget* w;
    // -----------
    void Show()
    {
      gtk_widget_show(GTK_WIDGET(w));
    }
    void ShowAll()
    {
      gtk_widget_show_all(GTK_WIDGET(w));
    }
    void Hide()
    {
      gtk_widget_hide(GTK_WIDGET(w));
    }
    const std::string Name()
    {
      return std::string(gtk_widget_get_name(GTK_WIDGET(w)));
    }
    /// Gtk documentation states this method works
    /// only on widgets that can be activated
    int Activate()
    {
      return gtk_widget_activate(GTK_WIDGET(w));
    }
    operator GtkWidget*()
    {
      return GTK_WIDGET(w);
    }
    Widget() { }
    Widget(const Widget& other):w(GTK_WIDGET(other.w)) { }
    Widget(GtkWidget* o): w(o) { }
    virtual ~Widget() {}
  };
  class Container: public virtual Widget
  {
  public:
    void Add(Widget o)
    {
      gtk_container_add(GTK_CONTAINER(w), o);
    }
    void Remove(Widget o)
    {
      gtk_container_remove(GTK_CONTAINER(w), o);
    }
  };
  class ScrolledWindow: public virtual Container
  {
  public:
    Widget GetChild()
    {
      return Widget(gtk_bin_get_child(GTK_BIN(w)));
    }
    ScrolledWindow()
    {
      gtk_widget_set_halign(GTK_WIDGET(w), GTK_ALIGN_FILL);
      gtk_widget_set_valign(GTK_WIDGET(w), GTK_ALIGN_FILL);
    }
  };
  class Builder
  {
    GtkBuilder* b;
  public:
    Builder() { }
    Builder(GtkBuilder* builder): b(builder)  { }
    Builder(const std::string& file)
    {
      b= gtk_builder_new_from_file(file.c_str());
    }
    Widget GetWidget(const std::string& n)
    {
      return GTK_WIDGET(
        gtk_builder_get_object(b, n.c_str()));
    }
    operator GtkBuilder*()
    {
      return b;
    }
  };
};

#endif // _GTKPP_
