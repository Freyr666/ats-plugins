extern crate glib;
#[macro_use]
extern crate gst_plugin;
#[macro_use]
extern crate gstreamer as gst;
extern crate gstreamer_audio as gst_audio;
extern crate gstreamer_base as gst_base;
extern crate gstreamer_video as gst_video;

mod soundbar;

fn plugin_init(plugin: &gst::Plugin) -> bool {
    soundbar::register(plugin);
    true
}

plugin_define!(
    b"ats-renderers\0",
    b"Rendering Plugin\0",
    plugin_init,
    b"1.0\0",
    b"MIT/X11\0",
    b"ats-renderers\0",
    b"ats-renderers\0",
    b"https://github.com/Freyr666/ats-plugins\0",
    b"2018-04-26\0"
);
