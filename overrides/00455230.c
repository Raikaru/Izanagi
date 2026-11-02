/* H_Cdvd_SelectDirectory (retail 0x00455230). Retail looks the normalized directory up in two
 * tables (D_007107b0 for the BTL volume, D_00711190 for the main volume, "VOL") and hands the
 * matching "#DirLst#" blob to the ROFS driver (func_004f2798), which makes that directory's files
 * reachable by name on that volume and only those: the driver holds one file list per volume.
 * The host VFS (src/platform/files) holds the whole tree, so the port remembers the volume's
 * current directory (p4pc_cdvd_set_dir) and the ROFS/ADXF name lookup (p4pc_adxf_resolve_name)
 * resolves "VOL:NAME" / "BTL:NAME" inside it. A directory in neither table is reported and
 * changes nothing, as retail's does. */
extern void p4pc_cdvd_set_dir(const char *dir, int volume);

void H_Cdvd_SelectDirectory(const char* dir)
{
    char normalized[256];
    s16 i;

    H_Cdvd_NormalizePath(dir, normalized);
    for (i = 0; i < 300 && D_007107b0[i][0] != '\0'; i++)
    {
        if (strcmp(D_007107b0[i], normalized + 4) == 0)
        {
            p4pc_cdvd_set_dir(normalized + 4, 1);
            return;
        }
    }
    for (i = 0; i < 300 && D_00711190[i][0] != '\0'; i++)
    {
        if (strcmp(D_00711190[i], normalized + 4) == 0)
        {
            p4pc_cdvd_set_dir(normalized + 4, 0);
            return;
        }
    }
    func_00440b68(D_00711630, normalized + 4);
}
