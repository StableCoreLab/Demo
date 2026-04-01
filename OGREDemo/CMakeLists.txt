cmake_minimum_required(VERSION 3.24)

project(OGREDemo LANGUAGES CXX)

# ===== 依赖项 =====
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 自动配置
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ===== 路径设置 =====
set(OGRE_NEXT_ROOT "D:/code/StableCore/ThirdParty/ogre-next")
set(OGRE_BUILD_DIR "${OGRE_NEXT_ROOT}/build")
set(OGRE_LIB_ROOT "D:/code/StableCore/ThirdParty/lib")
set(OGRE_INCLUDE_DIRS
    "${OGRE_NEXT_ROOT}/OgreMain/include"
	"${OGRE_NEXT_ROOT}/OgreMain/include/Compositor"
    "${OGRE_BUILD_DIR}/include"
)

# ===== Qt =====
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# ===== 源文件 =====
set(SOURCES
    main.cpp
    OGREDemo.cpp
    OGREDemo.h
    OgreWidget.cpp
    OgreWidget.h
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt6::Widgets
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${OGRE_INCLUDE_DIRS}
)

# ===== 多配置库目录（x64 架构）=====
target_link_directories(${PROJECT_NAME}
    PRIVATE
        "$<$<CONFIG:Debug>:${OGRE_LIB_ROOT}/Debug/x64>"
        "$<$<CONFIG:Release>:${OGRE_LIB_ROOT}/Release/x64>"
        "$<$<CONFIG:RelWithDebInfo>:${OGRE_LIB_ROOT}/RelWithDebInfo/x64>"
        "$<$<CONFIG:MinSizeRel>:${OGRE_LIB_ROOT}/Release/x64>"
)

# ===== OGRE 链接库（动态）=====
# 根据配置类型选择不同库名替换
target_link_libraries(${PROJECT_NAME}
    PRIVATE
		Qt6::Widgets
        $<$<CONFIG:Debug>:OgreNextMain_d>
        $<$<CONFIG:Release>:OgreNextMain>
        $<$<CONFIG:RelWithDebInfo>:OgreNextMain>
        $<$<CONFIG:MinSizeRel>:OgreNextMain>
)


# ===== 输出目录统一 =====
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin"
)

# ===== VS 调试目录 =====
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# ===== 复制 plugins.cfg =====
set(OGRE_PLUGIN_CFG "${CMAKE_CURRENT_SOURCE_DIR}/plugins.cfg")

if(EXISTS "${OGRE_PLUGIN_CFG}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OGRE_PLUGIN_CFG}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins.cfg"
        COMMENT "Copy plugins.cfg"
    )
else()
    message(WARNING "plugins.cfg not found: ${OGRE_PLUGIN_CFG}")
endif()

# ===== 自动复制 DLL文件到运行目录（根据编译配置）=====
set(OGRE_DLL_ROOT "D:/code/StableCore/ThirdParty/bin")

# 根据CONFIG类型选择DLL分支
if(CMAKE_CONFIGURATION_TYPES)
    # Visual Studio 多配置生成器（Debug/Release/RelWithDebInfo等）
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OGRE_DLL_ROOT}/$<CONFIG>/x64/OgreNextMain.dll"
            "${OGRE_DLL_ROOT}/$<CONFIG>/x64/RenderSystem_Direct3D11.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copy OGRE DLLs for $<CONFIG>"
    )
else()
    # 单配置生成器
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OGRE_DLL_ROOT}/${CMAKE_BUILD_TYPE}/x64/OgreNextMain.dll"
            "${OGRE_DLL_ROOT}/${CMAKE_BUILD_TYPE}/x64/RenderSystem_Direct3D11.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copy OGRE DLLs"
    )
endif()

# ===== ���飨VS��������=====
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})