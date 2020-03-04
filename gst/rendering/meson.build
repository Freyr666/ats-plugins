rendering_sources = [
  'gstatsrendering.c',
  'gldrawing.c',
  'gstglsoundbar.c',
]

gstrendering = library('gstatsrendering',
  rendering_sources,
  c_args : gstreamer_ats_args + ['-DGST_USE_UNSTABLE_API'],
  include_directories : [configinc],
  dependencies : [gstbase_dep, gstaudio_dep, gstpbutils_dep,
    gstgl_dep, libdrm_dep, libudev_dep, egl_dep, gl_dep, glesv2_dep,
    libm],
  install : true,
  install_dir : plugins_install_dir,
)

plugins = [gstrendering]