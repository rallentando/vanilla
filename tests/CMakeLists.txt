# the tests link 'vanilla_core', so they compile against the same headers and
# the same feature macros the executable is built with.
find_package(Qt6 6.5 REQUIRED COMPONENTS Test)

# lupdate reads every target of this project. nothing here holds a string a
# translator should ever see.
set_property(DIRECTORY PROPERTY QT_EXCLUDE_FROM_TRANSLATION ON)

# every test is one executable with one 'QTEST_MAIN'.
function(vanilla_add_test name)
    qt_add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE vanilla_core Qt6::Test)

    # 'Application::BaseDirectory()' is the directory of the running executable,
    # so a test which lets the application write ends up writing next to itself.
    # give each one a directory of its own, and the tests cannot collide.
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${name}")

    add_test(NAME ${name} COMMAND ${name})

    # 'offscreen': nothing below opens a window, and CI has no display.
    #
    # 'QT_FORCE_STDERR_LOGGING': without it Qt writes the test log to
    # 'OutputDebugString' instead of the console, and 'ctest
    # --output-on-failure' has nothing to show.
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_FORCE_STDERR_LOGGING=1")
endfunction()

vanilla_add_test(tst_treeserializer)
vanilla_add_test(tst_treescale)
vanilla_add_test(tst_viewnode)
vanilla_add_test(tst_inputmapping)
vanilla_add_test(tst_gadgetsscroll)
vanilla_add_test(tst_gadgetsframe)
# 'vanilla.qrc' for the tree glyphs ('folder' / 'folded' / 'unfolded') a
# directory card falls back to; without it the styles draw no glyph at all.
target_sources(tst_gadgetsframe PRIVATE ../qrc/vanilla.qrc)
vanilla_add_test(tst_theme)
vanilla_add_test(tst_pagetheme)
vanilla_add_test(tst_uimetrics)
vanilla_add_test(tst_windowframe)
vanilla_add_test(tst_receiver)
vanilla_add_test(tst_settingsschema)
vanilla_add_test(tst_mediatype)
vanilla_add_test(tst_thumbnail)
vanilla_add_test(tst_bookmarkio)
vanilla_add_test(tst_settingsio)
vanilla_add_test(tst_inputmap)
vanilla_add_test(tst_inputmapschema)
vanilla_add_test(tst_cookiefilter)
vanilla_add_test(tst_urlblock)
vanilla_add_test(tst_dnrrules)
vanilla_add_test(tst_cdpshims)
# (the shims' tests outgrew the sections an MSVC object may hold: C1128.)
if(MSVC)
    target_compile_options(tst_cdpshims PRIVATE /bigobj)
endif()
# (and the table of what the shims answer is held against the document which shows it.)
target_compile_definitions(tst_cdpshims PRIVATE
    VANILLA_DOCS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../docs")
vanilla_add_test(tst_extensioncopy)
vanilla_add_test(tst_extensionhostwire)
vanilla_add_test(tst_extensionmainscripts)
vanilla_add_test(tst_extensionuserscripts)
vanilla_add_test(tst_extensionui)
vanilla_add_test(tst_searchengine)
vanilla_add_test(tst_vanillapage)
vanilla_add_test(tst_directorypage)
vanilla_add_test(tst_devicescale)
vanilla_add_test(tst_commandmap)
vanilla_add_test(tst_commandframe)
vanilla_add_test(tst_windowledger)
vanilla_add_test(tst_useragent)
vanilla_add_test(tst_profilename)
vanilla_add_test(tst_viewinterface)
vanilla_add_test(tst_certificatepolicy)
vanilla_add_test(tst_fileoperation)
vanilla_add_test(tst_dialogbuttons)
vanilla_add_test(tst_nativehistory)
vanilla_add_test(tst_quickviewbridge)
vanilla_add_test(tst_jscode)
vanilla_add_test(tst_edgestate)
vanilla_add_test(tst_edgehostwindow)
vanilla_add_test(tst_spentbuttons)
if(TARGET Qt6::WebEngineWidgets)
    target_sources(tst_spentbuttons PRIVATE ../qrc/quickwebengineview6.qrc)
endif()
# the two profiles of a space, with real views of both kinds (D-428).
vanilla_add_test(tst_profilepair)
if(TARGET Qt6::WebEngineWidgets)
    target_sources(tst_profilepair PRIVATE ../qrc/quickwebengineview6.qrc)
endif()
vanilla_add_test(tst_zoomsave)
vanilla_add_test(tst_minimap)
vanilla_add_test(tst_downloadname)
vanilla_add_test(tst_downloaditem)
vanilla_add_test(tst_sslprotocol)
vanilla_add_test(tst_treebaritem)
vanilla_add_test(tst_loadending)
vanilla_add_test(tst_translationorder)
# the .qm files the application ships, read where the build puts them.
target_compile_definitions(tst_translationorder PRIVATE
    VANILLA_QM_DIR="${VANILLA_QM_DIR}")
add_dependencies(tst_translationorder vanilla)
vanilla_add_test(tst_shutdown)
vanilla_add_test(tst_extensions)
target_compile_definitions(tst_extensions PRIVATE
    VANILLA_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/extension")

# Explicit desktop smoke test, not a headless ctest. Its stores live beside
# its own executable and it serves only loopback HTTP (D-313).
if(TARGET Qt6::WebEngineWidgets)
    # 'vanilla.qrc' for the bar's own bitmaps, which the screenshots show.
    qt_add_executable(extension_smoke extension_smoke.cpp
        ../qrc/quickwebengineview6.qrc ../qrc/vanilla.qrc)
    target_link_libraries(extension_smoke PRIVATE vanilla_core Qt6::Test)
    target_compile_definitions(extension_smoke PRIVATE
        VANILLA_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/extension")
    set_target_properties(extension_smoke PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/extension_smoke")
endif()

# 'tst_settingsschema' reads the sources to check that the defaults in the
# schema are the ones the code actually asks for, and 'tst_commandmap' reads
# the '.qml' to check the functions the editing actions call are there, so
# both have to know where the sources are: the test binaries run from the
# build tree.
target_compile_definitions(tst_settingsschema PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")
# ('tst_extensioncopy': that copies are made where an extension is added and nowhere else.)
target_compile_definitions(tst_extensioncopy PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")
target_compile_definitions(tst_commandmap PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")
# ('tst_extensionhostwire': that 'activeTab' is granted by the user alone, D-439.)
target_compile_definitions(tst_extensionhostwire PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")
# ('tst_inputmapschema': that the labels are the words the menus put on the actions.)
target_compile_definitions(tst_inputmapschema PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")
target_compile_definitions(tst_viewinterface PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")

# 'tst_cookiefilter' reads 'networkcontroller.cpp' to check that the mirror is
# wired to the engine's store and kept out of 'cookie.json'. Neither is a
# compile error when it is wrong, and neither can be reached without an
# engine (D-257).
target_compile_definitions(tst_cookiefilter PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")

# 'tst_urlblock' reads 'networkcontroller.cpp' to check that every engine
# profile is given the interceptor -- the rules exist for nobody if one kind
# of profile never gets it, and that is not a compile error (D-271).
target_compile_definitions(tst_urlblock PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")

# 'tst_edgestate' reads the Edge sources to check that every event registered
# is kept on the list it comes off, and that the three lists come off before
# the controller is closed (D-251). Neither is a compile error when it is
# wrong.
target_compile_definitions(tst_edgestate PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")

# 'tst_downloaditem' reads 'networkcontroller.cpp' to check that the tab whose
# navigation a download took over is asked of 'View' rather than of one kind
# of view: the quick view's download is a 'QWebEngineDownloadRequest' too, and
# the cast which was there silently never matched (D-280 追記6).
target_compile_definitions(tst_downloaditem PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")

# 'tst_viewnode' reads 'treebank.cpp' to check the order a download carrier is
# taken out of the tree in, and that it never passes through the trash
# (D-279). Neither is a compile error when it is wrong.
target_compile_definitions(tst_viewnode PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")

# 'tst_pagetheme' reads the two style sheets under 'resources/' to check that
# they hold no colour of their own, so it needs the same bearing.
target_compile_definitions(tst_pagetheme PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")

# 'tst_uimetrics' reads the painting code and both style sheets, to check that
# nothing rounds a corner by a number of its own.
target_compile_definitions(tst_uimetrics PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")

# 'tst_nativehistory' reads 'quicknativewebview.qml' to check the load status
# names it asks the backend for are names the backend has.
target_compile_definitions(tst_nativehistory PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")

# 'tst_jscode' reads the three drop handlers, to check that none of them
# escapes the dropped text before the script does (A-25).
target_compile_definitions(tst_jscode PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")
target_compile_definitions(tst_jscode PRIVATE
    VANILLA_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../resources")

# 'tst_quickviewbridge' reads 'quickwebengineview6.qml' and the C++ which
# calls into it, to check that no name on that seam is spelled at nothing and
# no function on it is empty.
target_compile_definitions(tst_quickviewbridge PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")
target_compile_definitions(tst_shutdown PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")
