#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <exception>
#include <functional>
// ----------------
#include <gtk/gtk.h>
// -----------------
#include <chaiscript/chaiscript.hpp>
#include <chaiscript/chaiscript_stdlib.hpp>
// -------------------
#include <SharedFile.hpp>
// -------------------
#include "include/GtkPP.hpp"
// --------------------------
#if defined(DEBUG) || defined(_DEBUG)
    #ifndef DBG_ONLY
      #define DBG_ONLY(...) do { __VA_ARGS__; } while (0)
    #endif
#else
    #ifndef DBG_ONLY
      #define DBG_ONLY(...)
    #endif
#endif

using namespace std;

using event_type = function<void(GtkPP::Widget)>;
void register_gtk_chai(chaiscript::ChaiScript&);

class Coral
{
  GtkCssProvider* css_prov;
  //GtkBuilder* builder;
  GtkWindow* window;
  GtkPP::Builder builder;
public:
  Coral():
    builder("Template/testbuilder3.glade"),
    chai(chaiscript::Std_Lib::library())
    {
    }
  GtkWidget* menu_quit;
  void Init()
  {
    register_gtk_chai(chai);
    css_prov = gtk_css_provider_new();
    LoadStyle();
    BuildWindow();
    gtk_widget_show(GTK_WIDGET(window));

    menu_quit = GTK_WIDGET(
      gtk_builder_get_object(builder,"menu_file_quit"));
    chai.eval<event_type>("widget_test")(menu_quit);
  }
  chaiscript::ChaiScript chai;
private:
  void LoadStyle();
  void BuildWindow();
};
chaiscript::ChaiScript* chai;

int main(int argc, char** argv) try
{
  gtk_init(&argc, &argv);

  unique_ptr<Coral> coral(new Coral());

  chai = &coral->chai;

  coral->Init();

  gtk_main();

  return 0;
}
catch(exception& e)
{
  puts(e.what());

  return EXIT_FAILURE;
}
vector<event_type> methods;
void wrapper(GtkWidget* w, void* n)
{
  auto index = reinterpret_cast<decltype(methods)::size_type>(n);
  DBG_ONLY( cout <<  index << endl; );

  methods[index](w);//methods[index](Glib::wrap(w));
}
extern "C" void signal_connector(GtkBuilder *builder, GObject *object, const gchar *signal_name,
  const gchar *handler_name, GObject *connect_object, GConnectFlags flags, gpointer user_data)
{
  DBG_ONLY(
    printf("registering handler: %s\n", handler_name);
  );

  methods.push_back(chai->eval<event_type>(handler_name));

  g_signal_connect(
    object,
    signal_name,
    G_CALLBACK(wrapper),
    reinterpret_cast<void*>(methods.size()-1));
}
void Coral::BuildWindow()
{
  //builder = gtk_builder_new_from_file("Template/testbuilder3.glade");
  window = GTK_WINDOW
    (gtk_builder_get_object(builder, "applicationwindow1"));

  chai.add(chaiscript::var(&builder), "builder");
  // We call this here so that the builder can
  // be exposed to this specific script
  chai.eval_file("script/callbacks.chai");

  gtk_builder_connect_signals_full(
     builder,
     signal_connector,
     NULL);

  g_signal_connect(window, "destroy",
          G_CALLBACK(gtk_main_quit),0);
}
void Coral::LoadStyle()
{
  auto file = SharedFile("style.css", "rb");
  auto f = file.get();
  // Find out the files size in bytes
  fseek(f, 0, SEEK_END);
  int fsize = ftell(f)+1;
  fseek(f, 0, SEEK_SET);
  // Load the file into a buffer
  unique_ptr<char[]> data(new char[fsize]);
  fread(data.get(), fsize, 1, f);

  if(gtk_css_provider_load_from_data(css_prov, data.get(), fsize, nullptr))
  {
    auto default_screen = gdk_display_get_default_screen(
                            gdk_display_get_default());

    gtk_style_context_add_provider_for_screen(default_screen,
                              GTK_STYLE_PROVIDER(css_prov), 800);
  }
}
