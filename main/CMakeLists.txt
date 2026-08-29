idf_component_register(SRCS "LCD1602.c" "canSender.c" "main.c"
                       PRIV_REQUIRES spi_flash
                       INCLUDE_DIRS "")
