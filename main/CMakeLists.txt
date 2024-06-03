# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c")
set(COMPONENT_ADD_INCLUDEDIRS "")

idf_component_register(SRCS "main.c"
	INCLUDE_DIRS "."
)

add_custom_target(dep COMMAND cd ../../../main/web && python3 compile.py)

spiffs_create_partition_image(spiffs web/rendered FLASH_IN_PROJECT DEPENDS dep)
