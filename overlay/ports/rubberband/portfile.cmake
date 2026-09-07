vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO breakfastquay/rubberband
    REF "v${VERSION}"
    SHA512 f581e900a71f78fde3361d2bed2fe165952c2ca087168c5f4e4994586bd832267eea58e0662a74b6a7430bc361fe80b5307b2ee6bf631a3561a8cba86e1cd3f2
    HEAD_REF default
)

vcpkg_configure_meson(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -Dfft=builtin
        -Dresampler=libsamplerate
        -Djni=disabled
        -Dladspa=disabled
        -Dlv2=disabled
        -Dvamp=disabled
        -Dcmdline=disabled
        -Dtests=disabled
    )

vcpkg_install_meson()
vcpkg_fixup_pkgconfig()
vcpkg_copy_pdbs()
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/COPYING")
