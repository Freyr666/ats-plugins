analysis_sources = [
  'gstanalysis.c',
]

gstanalysis = library('gstanalysis',
  analysis_sources,
  c_args : gstreamer_ats_args + ['-DGST_USE_UNSTABLE_API'],
  include_directories : [configinc],
  dependencies : [gstbase_dep, gstvideo_dep, gstaudio_dep, gstpbutils_dep,
    gstgl_dep, libm],
  install : true,
  install_dir : plugins_install_dir,
)

plugins = [gstanalysis]