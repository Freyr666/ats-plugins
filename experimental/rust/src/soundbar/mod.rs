use glib;
use gst;
use gst::prelude::*;
use gst_video;
use gst_audio;
 
use gst_plugin::properties::*;
use gst_plugin::object::*;
use gst_plugin::element::*;
use gst_plugin::base_transform::*;
use gst_plugin::adapter::*;
 
use std::i32;
use std::sync::Mutex;

struct State {
    pub adapter: Adapter,
}

impl State {
    fn new () -> State {
        State { adapter: Adapter::new() }
    }
}

struct Soundbar {
    cat:     gst::DebugCategory,
    state:   Mutex<State>,
    srcpad:  gst::Pad,
    sinkpad: gst::Pad,
}

impl Soundbar {

    // TODO implement
    fn src_event (pad: &gst::Pad, soundbar: &Option<gst::Object>, event: gst::Event) -> bool {
        true
    }

    // TODO implement flush_stop and 
    fn sink_event (pad: &gst::Pad, soundbar: &Option<gst::Object>, event: gst::Event) -> bool {
        true
    }
    
    // TODO implement
    fn src_chain (pad: &gst::Pad, soundbar: &Option<gst::Object>, buffer: gst::Buffer) -> gst::FlowReturn {
        gst::FlowReturn::Ok
    }

    // TODO implement latency query
    fn sink_query (pad: &gst::Pad, soundbar: &Option<gst::Object>, query: gst::Query) -> bool {
        false
    }
    
    fn new(transform: &BaseTransform) -> Box<BaseTransformImpl<BaseTransform>> {
        let template = transform.get_pad_template("src").unwrap();
        let srcpad   = gst::Pad::new_from_template(&template, None);
        srcpad.set_chain_function( Self::src_chain );
        srcpad.set_event_function( Self::src_event );
        transform.add_pad(&srcpad).unwrap();
        
        let template = transform.get_pad_template("sink").unwrap();
        let sinkpad  = gst::Pad::new_from_template(&template, None);
        srcpad.set_query_function( Self::sink_query );
        srcpad.set_event_function( Self::sink_event );
        transform.add_pad(&sinkpad).unwrap();
        
        Box::new(Self {
            cat: gst::DebugCategory::new(
                "soundbar",
                gst::DebugColorFlags::empty(),
                "Soundbar renderer",
            ),
            state: Mutex::new(State::new()),
            srcpad,
            sinkpad,
        })
    }
    
    fn class_init(klass: &mut BaseTransformClass) {
        
        klass.set_metadata(
            "Soundbar renderer",
            "Filter/Effect/Converter/Video",
            "Renders soundbar for the given audio src",
            "Freyr <sky_rider_93@mail.ru>",
        );

        let caps = gst::Caps::new_simple(
            "video/x-raw",
            &[
                (
                    "format",
                    &gst::List::new(&[
                        &gst_video::VideoFormat::Bgrx.to_string(),
                    ]),
                ),
                ("width", &gst::IntRange::<i32>::new(0, i32::MAX)),
                ("height", &gst::IntRange::<i32>::new(0, i32::MAX)),
                (
                    "framerate",
                    &gst::FractionRange::new(
                        gst::Fraction::new(0, 1),
                        gst::Fraction::new(i32::MAX, 1),
                    ),
                ),
            ],
        );
        
        let src_pad_template = gst::PadTemplate::new(
            "src",
            gst::PadDirection::Src,
            gst::PadPresence::Always,
            &caps,
        );
        klass.add_pad_template(src_pad_template);

        let caps = gst::Caps::new_simple(
            "audio/x-raw",
            &[
                (
                    "format",
                    &gst::List::new(&[
                        &gst_audio::AUDIO_FORMAT_F32.to_string(),
                        &gst_audio::AUDIO_FORMAT_F64.to_string(),
                    ]),
                ),
                ("layout", &"interleaved"),
                ("rate", &gst::IntRange::<i32>::new(1, i32::MAX)),
                ("channels", &gst::IntRange::<i32>::new(1, i32::MAX)),
            ],
        );
        
        let sink_pad_template = gst::PadTemplate::new(
            "sink",
            gst::PadDirection::Sink,
            gst::PadPresence::Always,
            &caps,
        );
        klass.add_pad_template(sink_pad_template);
        
        klass.configure(BaseTransformMode::NeverInPlace, false, false);
    }
}

impl ObjectImpl<BaseTransform> for Soundbar {}

impl ElementImpl<BaseTransform> for Soundbar {}

impl BaseTransformImpl<BaseTransform> for Soundbar {}

struct SoundbarStatic;

impl ImplTypeStatic<BaseTransform> for SoundbarStatic {
    fn get_name(&self) -> &str {
        "Soundbar"
    }
 
    fn new(&self, element: &BaseTransform) -> Box<BaseTransformImpl<BaseTransform>> {
        Soundbar::new(element)
    }
 
    fn class_init(&self, klass: &mut BaseTransformClass) {
        Soundbar::class_init(klass);
    }
}
 
pub fn register(plugin: &gst::Plugin) {
    let type_ = register_type(SoundbarStatic);
    gst::Element::register(plugin, "soundbar", 0, type_);
}
