SET(IWIDGETS_PKGNAME Iwidgets)
SET(IWIDGETS_PKGVERSION "4.0.1")
SET(VERSION ${IWIDGETS_PKGVERSION})
SET(IWIDGETS_VERSION ${IWIDGETS_PKGVERSION})

SET(IWIDGETS_SCRIPTS
  generic/buttonbox.itk
  generic/calendar.itk
  generic/canvasprintbox.itk
  generic/canvasprintdialog.itk
  generic/checkbox.itk
  generic/colors.itcl
  generic/combobox.itk
  generic/dateentry.itk
  generic/datefield.itk
  generic/dialog.itk
  generic/dialogshell.itk
  generic/disjointlistbox.itk
  generic/entryfield.itk
  generic/extbutton.itk
  generic/extfileselectionbox.itk
  generic/extfileselectiondialog.itk
  generic/feedback.itk
  generic/fileselectionbox.itk
  generic/fileselectiondialog.itk
  generic/finddialog.itk
  generic/hierarchy.itk
  generic/hyperhelp.itk
  generic/labeledframe.itk
  generic/labeledwidget.itk
  generic/mainwindow.itk
  generic/menubar.itk
  generic/messagebox.itk
  generic/messagedialog.itk
  generic/notebook.itk
  generic/optionmenu.itk
  generic/panedwindow.itk
  generic/pane.itk
  generic/promptdialog.itk
  generic/pushbutton.itk
  generic/radiobox.itk
  generic/regexpfield.itk
  generic/roman.itcl
  generic/scopedobject.itcl
  generic/scrolledcanvas.itk
  generic/scrolledframe.itk
  generic/scrolledhtml.itk
  generic/scrolledlistbox.itk
  generic/scrolledtext.itk
  generic/scrolledwidget.itk
  generic/selectionbox.itk
  generic/selectiondialog.itk
  generic/shell.itk
  generic/spindate.itk
  generic/spinint.itk
  generic/spinner.itk
  generic/spintime.itk
  generic/tabnotebook.itk
  generic/tabset.itk
  generic/tclIndex
  generic/timeentry.itk
  generic/timefield.itk
  generic/toolbar.itk
  generic/unknownimage.gif
  generic/watch.itk
  )

FOREACH(script ${IWIDGETS_SCRIPTS})
  install(FILES ${script} DESTINATION lib/${IWIDGETS_PKGNAME}${IWIDGETS_PKGVERSION}/scripts)
ENDFOREACH(script ${IWIDGETS_SCRIPTS})
CMAKEFILES(${IWIDGETS_SCRIPTS})

FOREACH(file ${IWIDGETS_SCRIPTS})
  get_filename_component(filename ${file} NAME)
  if(NOT CMAKE_CONFIGURATION_TYPES)
    configure_file(${file} ${CMAKE_BINARY_DIR}/lib/${IWIDGETS_PKGNAME}${IWIDGETS_PKGVERSION}/scripts/${filename} COPYONLY)
  else(NOT CMAKE_CONFIGURATION_TYPES)
    foreach(CFG_TYPE ${CMAKE_CONFIGURATION_TYPES})
      string(TOUPPER "${CFG_TYPE}" CFG_TYPE_UPPER)
      configure_file(${file} ${CMAKE_BINARY_DIR_${CFG_TYPE_UPPER}}/lib/${IWIDGETS_PKGNAME}${IWIDGETS_PKGVERSION}/scripts/${filename} COPYONLY)
    endforeach(CFG_TYPE ${CMAKE_CONFIGURATION_TYPES})
  endif(NOT CMAKE_CONFIGURATION_TYPES)
ENDFOREACH(file ${ITK_LIBRARY_FILES})

# Create the pkgIndex.tcl file and configure iwidgets.tcl.
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl "package ifneeded ${IWIDGETS_PKGNAME} ${IWIDGETS_PKGVERSION} [list source [file join $dir iwidgets.tcl]]")
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl PROPERTIES GENERATED TRUE)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/iwidgets.tcl.in ${CMAKE_CURRENT_BINARY_DIR}/iwidgets.tcl)

# Place files in build directory
if(NOT CMAKE_CONFIGURATION_TYPES)
  configure_file(${CMAKE_CURRENT_BINARY_DIR}/iwidgets.tcl ${CMAKE_BINARY_DIR}/lib/${IWIDGETS_PKGNAME}${IWIDGETS_PKGVERSION}/iwidgets.tcl COPYONLY)
  configure_file(${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl ${CMAKE_BINARY_DIR}/lib/${IWIDGETS_PKGNAME}${IWIDGETS_PKGVERSION}/pkgIndex.tcl COPYONLY)
else(NOT CMAKE_CONFIGURATION_TYPES)
  foreach(CFG_TYPE ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${CFG_TYPE}" CFG_TYPE_UPPER)
    configure_file(${CMAKE_CURRENT_BINARY_DIR}/iwidgets.tcl ${CMAKE_BINARY_DIR_${CFG_TYPE_UPPER}}/lib/${IWIDGETS_PKGNAME}${IWIDGETS_PKGVERSION}/iwidgets.tcl COPYONLY)
    configure_file(${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl ${CMAKE_BINARY_DIR_${CFG_TYPE_UPPER}}/lib/${IWIDGETS_PKGNAME}${IWIDGETS_PKGVERSION}/pkgIndex.tcl COPYONLY)
  endforeach(CFG_TYPE ${CMAKE_CONFIGURATION_TYPES})
endif(NOT CMAKE_CONFIGURATION_TYPES)

# Add install rules
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl DESTINATION lib/${IWIDGETS_PKGNAME}${IWIDGETS_PKGVERSION})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/iwidgets.tcl DESTINATION lib/${IWIDGETS_PKGNAME}${IWIDGETS_PKGVERSION})
install(FILES license.terms DESTINATION lib/${IWIDGETS_PKGNAME}${IWIDGETS_PKGVERSION})

ADD_SUBDIRECTORY(doc)
