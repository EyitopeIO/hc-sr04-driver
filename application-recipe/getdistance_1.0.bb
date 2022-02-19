SUMMARY = "Get distance using the hc-sr04 ultrasonic range sensor module"
DESCRIPTION = "Recipe to build getdistance.c"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
SRC_URI = "file://getdistance.c"

# Arguments to linker
# TARGET_CC_ARCH += "${LDFLAGS}"

do_compile() {
    ${CC} -o getdistance getdistance.c
}

do_install() {
    install -d ${D}${bindir}
    install -m 0777 getdistance ${D}${bindir}
}