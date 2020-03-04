project('gstreamer-ats-plugins', 'c',
  version : '1.16.2',
  meson_version : '>= 0.47.0',
  default_options : [ 'warning_level=1',
                      'buildtype=debugoptimized' ])

gst_version = meson.project_version()
version_arr = gst_version.split('.')
gst_version_major = version_arr[0].to_int()
gst_version_minor = version_arr[1].to_int()
gst_version_micro = version_arr[2].to_int()

glib_req = '>= 2.40.0'
gst_req = '>= @0@.@1@.0'.format(gst_version_major, gst_version_minor)

cc = meson.get_compiler('c')

if cc.has_link_argument('-Wl,-Bsymbolic-functions')
  add_project_link_arguments('-Wl,-Bsymbolic-functions', language : 'c')
endif

# Symbol visibility
if cc.has_argument('-fvisibility=hidden')
  add_project_arguments('-fvisibility=hidden', language: 'c')
endif

# Disable strict aliasing
if cc.has_argument('-fno-strict-aliasing')
  add_project_arguments('-fno-strict-aliasing', language: 'c')
endif

# Mandatory GST deps
libm = cc.find_library('m', required : false)
gst_dep = dependency('gstreamer-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_dep'])
gstbase_dep = dependency('gstreamer-base-1.0', version : gst_req,
  fallback : ['gstreamer', 'gst_base_dep'])
gstpbutils_dep = dependency('gstreamer-pbutils-1.0', version : gst_req,
  fallback : ['gst-plugins-base', 'pbutils_dep'])
gstvideo_dep = dependency('gstreamer-video-1.0', version : gst_req,
  fallback : ['gst-plugins-base', 'video_dep'])
gstaudio_dep = dependency('gstreamer-audio-1.0', version : gst_req,
  fallback : ['gst-plugins-base', 'audio_dep'])
gstgl_dep = dependency('gstreamer-gl-1.0', version : gst_req,
  fallback : ['gst-plugins-base', 'gstgl_dep'], required: false)

libdrm_dep = dependency('libdrm', required: false)
libudev_dep = dependency('libudev', required: false)
egl_dep = dependency('egl', required: false)
gl_dep = dependency('gl', required: false)
glesv2_dep = dependency('glesv2', required: false)

GLES_VERSION_MASK = gl_dep.found() ? 1 : 0
if glesv2_dep.found()
  if (cc.has_header('GLES2/gl2.h', dependencies: glesv2_dep) and
      cc.has_header('GLES2/gl2ext.h', dependencies: glesv2_dep))
    GLES_VERSION_MASK += 4
  endif
  if (cc.has_header('GLES3/gl3.h', dependencies: glesv2_dep) and
      cc.has_header('GLES3/gl3ext.h', dependencies: glesv2_dep) and
      cc.has_header('GLES2/gl2ext.h', dependencies: glesv2_dep))
    GLES_VERSION_MASK += 8
  endif
endif

# Disable compiler warnings for unused variables and args if gst debug system is disabled
if gst_dep.type_name() == 'internal'
  gst_debug_disabled = not subproject('gstreamer').get_variable('gst_debug')
else
  # We can't check that in the case of subprojects as we won't
  # be able to build against an internal dependency (which is not built yet)
  gst_debug_disabled = cc.has_header_symbol('gst/gstconfig.h', 'GST_DISABLE_GST_DEBUG', dependencies: gst_dep)
endif

if gst_debug_disabled
  message('GStreamer debug system is disabled')
  add_project_arguments(cc.get_supported_arguments(['-Wno-unused']), language: 'c')
else
  message('GStreamer debug system is enabled')
endif

cdata = configuration_data()
cdata.set('PACKAGE_VERSION', '"@0@"'.format(gst_version))
cdata.set('PACKAGE', '"gst-plugins-ats"')
cdata.set('PACKAGE_BUGREPORT', '"https://github.com/Freyr666/ats-plugins/issues"')

api_version = '1.0'
soversion = 0
# maintaining compatibility with the previous libtool versioning
# current = minor * 100 + micro
curversion = gst_version_minor * 100 + gst_version_micro
libversion = '@0@.@1@.0'.format(soversion, curversion)
osxversion = curversion + 1

plugins_install_dir = '@0@/gstreamer-1.0'.format(get_option('libdir'))

configure_file(configuration: cdata, output: 'config.h')
gstreamer_ats_args = ['-DHAVE_CONFIG_H']
configinc = include_directories('.')

subdir('gst')