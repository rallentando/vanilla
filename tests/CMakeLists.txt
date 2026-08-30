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
vanilla_add_test(tst_theme)
vanilla_add_test(tst_pagetheme)
vanilla_add_test(tst_uimetrics)
vanilla_add_test(tst_settingsschema)
vanilla_add_test(tst_mediatype)
vanilla_add_test(tst_thumbnail)
vanilla_add_test(tst_bookmarkio)
vanilla_add_test(tst_settingsio)
vanilla_add_test(tst_inputmap)
vanilla_add_test(tst_cookiefilter)
vanilla_add_test(tst_urlblock)
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
vanilla_add_test(tst_minimap)
vanilla_add_test(tst_downloadname)
vanilla_add_test(tst_downloaditem)
vanilla_add_test(tst_sslprotocol)

# 'tst_settingsschema' reads the sources to check that the defaults in the
# schema are the ones the code actually asks for, and 'tst_commandmap' reads
# the '.qml' to check the functions the editing actions call are there, so
# both have to know where the sources are: the test binaries run from the
# build tree.
target_compile_definitions(tst_settingsschema PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")
target_compile_definitions(tst_commandmap PRIVATE
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

# 'tst_quickviewbridge' reads 'quickwebengineview6.qml' and the C++ which
# calls into it, to check that no name on that seam is spelled at nothing and
# no function on it is empty.
target_compile_definitions(tst_quickviewbridge PRIVATE
    VANILLA_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../src")
