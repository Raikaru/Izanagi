# M8f discovery boundary: adjacent non-EPL formats

The4400-object core census is not a claim that every graphical dependency is an EPL/EP. A complete inventory-extension/signature scan also found the following adjacent candidates. These rows are additional metadata, **not added to the4400 BED/EPL/EP/PB core count**.

## Classification evidence

- `system/effect/effpack.bin`: nine locale/neutral occurrences, one PAK-v2 identity,22 members named GENERAL_00.tmx through GENERAL_21.tmx,208540 decimal bytes. The inventory signature label `rw-chunk-16` is only a first-word signature collision (count22), **not RenderWare grammar**; the bounded existing PAK reader recognizes the full object. All22 members are TMX texture dependencies, not EP descriptors or shader instructions. Texture decode belongs to M8a.
- `.BVP`:36 named occurrences, including battle/BSV105.BVP andBSV108.BVP plus packed copies/locales. BSV105.BVP contains12-byte index records with format1,offset,size. At its first inspected payload offset0x1770 the bytes+8 areMSG1, and the first three inspected payloads likewise carry MSG1. This is direct evidence of message payloads, not a particle/EPL header. This lane does not claim an exhaustive BVP parser or a voice/audio grammar from its filename. The all-BVP internal semantics are outside this battle-effects parser scope.
- `.track`: nine occurrences of the same96-byte `init.bin::insect/insect.bin::weapon.track` identity. Header bytes0..3 areTRCK, u32+4=0x54, u32+8=1, with anotherTRCK/0x54/1 header at+0x54. No named battle-root TRACK object occurs. Its full field semantics and legal battle linkage are **OPEN O06**, not silently declared another EPL or a supported sword-trail effect. No complete TRCK grammar is claimed from one object.
- `.gxp` shader blobs, `.dds`/`.tmx` texture resources, `.amd`/`.gmo`/`.pac` model graphs, camera/environment records and audio/script resources are separate dependencies handled by their existing owners. Names such as PLG0 alone are not evidence of a particle format; inspected PLG0 examples are facility/camp UI plugin assets. Unrecognized unnamed containers and M8c opaque effect suffixes remain explicit O06, not a certified absence.

## Counts and extracted identities

| CPK | TRACK | BVP | effpack.bin |
|---|---:|---:|---:|
| data.cpk | 1 | 4 | 1 |
| data_c.cpk | 1 | 4 | 1 |
| data_ck.cpk | 1 | 4 | 1 |
| data_de.cpk | 1 | 4 | 1 |
| data_e.cpk | 1 | 4 | 1 |
| data_es.cpk | 1 | 4 | 1 |
| data_fr.cpk | 1 | 4 | 1 |
| data_it.cpk | 1 | 4 | 1 |
| data_k.cpk | 1 | 4 | 1 |

| Path | Bytes | Extracted SHA-256 |
|---|---:|---|
| `data.cpk::battle/battle_pack.bin::BSV105.BVP` | 98528 | `8e3eb1dcb5590e3e1e2747d47ef53d99ac83aa37760a1403e6fff905599244c9` |
| `data.cpk::battle/battle_pack2.bin::BSV108.BVP` | 95344 | `530be00690a18d6c608b619ddc859c70bb857e9fd1ef71bd40fd71f4587c8df7` |
| `data.cpk::battle/BSV105.BVP` | 98528 | `8e3eb1dcb5590e3e1e2747d47ef53d99ac83aa37760a1403e6fff905599244c9` |
| `data.cpk::battle/BSV108.BVP` | 95344 | `530be00690a18d6c608b619ddc859c70bb857e9fd1ef71bd40fd71f4587c8df7` |
| `data.cpk::init.bin::insect/insect.bin::weapon.track` | 96 | `ee9843246fcfb03b6838114662756bbf7ef512cad1a1f7696fe017da4d6ba8e2` |
| `data.cpk::init_free.bin::system/effect/effpack.bin` | 208540 | `5d044b2b1060dd2caefdd1210fc093af274bd974d097f15732c865fcbd7c7b3b` |
| `data_c.cpk::battle/battle_pack.bin::BSV105.BVP` | 97232 | `3fde236ba2aae1bb98ee0dcdd7ce37fa05f09a6779fe076d6bbb3f7193bfa0b8` |
| `data_c.cpk::battle/battle_pack2.bin::BSV108.BVP` | 94336 | `914fa0cfc132a72ca397f5e5d005377bd211644a69c2f800b58c076fc3928b38` |
| `data_c.cpk::battle/BSV105.BVP` | 97232 | `3e186c6f144de44e4ec78f4afd60a9207934675179ed68531e79e5bc40ecde8f` |
| `data_c.cpk::battle/BSV108.BVP` | 94336 | `ba47bcca949386f927ca1b86464cc0465d798d49cdb5df427b9ce7f28b5cab4f` |
| `data_c.cpk::init.bin::insect/insect.bin::weapon.track` | 96 | `ee9843246fcfb03b6838114662756bbf7ef512cad1a1f7696fe017da4d6ba8e2` |
| `data_c.cpk::init_free.bin::system/effect/effpack.bin` | 208540 | `5d044b2b1060dd2caefdd1210fc093af274bd974d097f15732c865fcbd7c7b3b` |
| `data_ck.cpk::battle/battle_pack.bin::BSV105.BVP` | 96960 | `8e6f534660fba30a1d2987152a34173919a407c0fbfb11ac2fe5a274f1d517bd` |
| `data_ck.cpk::battle/battle_pack2.bin::BSV108.BVP` | 94096 | `7630d006a1427ca905f6d26b8952a31716e74b831371eae523ce71b6845d29af` |
| `data_ck.cpk::battle/BSV105.BVP` | 96960 | `8e6f534660fba30a1d2987152a34173919a407c0fbfb11ac2fe5a274f1d517bd` |
| `data_ck.cpk::battle/BSV108.BVP` | 94096 | `7630d006a1427ca905f6d26b8952a31716e74b831371eae523ce71b6845d29af` |
| `data_ck.cpk::init.bin::insect/insect.bin::weapon.track` | 96 | `ee9843246fcfb03b6838114662756bbf7ef512cad1a1f7696fe017da4d6ba8e2` |
| `data_ck.cpk::init_free.bin::system/effect/effpack.bin` | 208540 | `5d044b2b1060dd2caefdd1210fc093af274bd974d097f15732c865fcbd7c7b3b` |
| `data_de.cpk::battle/battle_pack.bin::BSV105.BVP` | 101296 | `4a1da8194433324eb552fccec9af04e8f1831317914ca9c88e64ea3a76c25ef8` |
| `data_de.cpk::battle/battle_pack2.bin::BSV108.BVP` | 96864 | `c93cd1d6ab46cbc0cbbb333eda7813b84d16839d426f9fed7c29d8b3ee6b4165` |
| `data_de.cpk::battle/BSV105.BVP` | 101296 | `4a1da8194433324eb552fccec9af04e8f1831317914ca9c88e64ea3a76c25ef8` |
| `data_de.cpk::battle/BSV108.BVP` | 96864 | `c93cd1d6ab46cbc0cbbb333eda7813b84d16839d426f9fed7c29d8b3ee6b4165` |
| `data_de.cpk::init.bin::insect/insect.bin::weapon.track` | 96 | `ee9843246fcfb03b6838114662756bbf7ef512cad1a1f7696fe017da4d6ba8e2` |
| `data_de.cpk::init_free.bin::system/effect/effpack.bin` | 208540 | `5d044b2b1060dd2caefdd1210fc093af274bd974d097f15732c865fcbd7c7b3b` |
| `data_e.cpk::battle/battle_pack.bin::BSV105.BVP` | 100960 | `0b60f45e5850d1057d72ff1fffeffc2311e1f952cba97d22157056c3887221ac` |
| `data_e.cpk::battle/battle_pack2.bin::BSV108.BVP` | 96144 | `dc560a82f831282fffbc51a3474329f1105221360585efacc9b9055308657b86` |
| `data_e.cpk::battle/BSV105.BVP` | 100960 | `0b60f45e5850d1057d72ff1fffeffc2311e1f952cba97d22157056c3887221ac` |
| `data_e.cpk::battle/BSV108.BVP` | 96144 | `dc560a82f831282fffbc51a3474329f1105221360585efacc9b9055308657b86` |
| `data_e.cpk::init.bin::insect/insect.bin::weapon.track` | 96 | `ee9843246fcfb03b6838114662756bbf7ef512cad1a1f7696fe017da4d6ba8e2` |
| `data_e.cpk::init_free.bin::system/effect/effpack.bin` | 208540 | `5d044b2b1060dd2caefdd1210fc093af274bd974d097f15732c865fcbd7c7b3b` |
| `data_es.cpk::battle/battle_pack.bin::BSV105.BVP` | 102176 | `578b70f97aca52a669008961a583effcca1350c31e765d40933ab58ed46bd7a6` |
| `data_es.cpk::battle/battle_pack2.bin::BSV108.BVP` | 97840 | `b8693e4d78bb032a7a1a250b36cb632e6e5e4ed6633f30b45c40e37890cad688` |
| `data_es.cpk::battle/BSV105.BVP` | 102176 | `578b70f97aca52a669008961a583effcca1350c31e765d40933ab58ed46bd7a6` |
| `data_es.cpk::battle/BSV108.BVP` | 97840 | `b8693e4d78bb032a7a1a250b36cb632e6e5e4ed6633f30b45c40e37890cad688` |
| `data_es.cpk::init.bin::insect/insect.bin::weapon.track` | 96 | `ee9843246fcfb03b6838114662756bbf7ef512cad1a1f7696fe017da4d6ba8e2` |
| `data_es.cpk::init_free.bin::system/effect/effpack.bin` | 208540 | `5d044b2b1060dd2caefdd1210fc093af274bd974d097f15732c865fcbd7c7b3b` |
| `data_fr.cpk::battle/battle_pack.bin::BSV105.BVP` | 101760 | `e45a70ca291d8a2488710c7c1fda3074a38ae81408008e745d13509a0a551814` |
| `data_fr.cpk::battle/battle_pack2.bin::BSV108.BVP` | 97632 | `3a5f1e7b01bc3453ebb6996299f7d20d126fdf5c2680d0112a466fd217faba39` |
| `data_fr.cpk::battle/BSV105.BVP` | 101760 | `e45a70ca291d8a2488710c7c1fda3074a38ae81408008e745d13509a0a551814` |
| `data_fr.cpk::battle/BSV108.BVP` | 97632 | `3a5f1e7b01bc3453ebb6996299f7d20d126fdf5c2680d0112a466fd217faba39` |
| `data_fr.cpk::init.bin::insect/insect.bin::weapon.track` | 96 | `ee9843246fcfb03b6838114662756bbf7ef512cad1a1f7696fe017da4d6ba8e2` |
| `data_fr.cpk::init_free.bin::system/effect/effpack.bin` | 208540 | `5d044b2b1060dd2caefdd1210fc093af274bd974d097f15732c865fcbd7c7b3b` |
| `data_it.cpk::battle/battle_pack.bin::BSV105.BVP` | 102352 | `6f29d678ea63b40bb58141bdec3a0ab7af31381ca226f0d8ff150104f0f4fe0d` |
| `data_it.cpk::battle/battle_pack2.bin::BSV108.BVP` | 97072 | `d1bedb721aa54b5d429121ff2ee8d312db100238f5ab554878cb952fda222e9a` |
| `data_it.cpk::battle/BSV105.BVP` | 102352 | `6f29d678ea63b40bb58141bdec3a0ab7af31381ca226f0d8ff150104f0f4fe0d` |
| `data_it.cpk::battle/BSV108.BVP` | 97072 | `d1bedb721aa54b5d429121ff2ee8d312db100238f5ab554878cb952fda222e9a` |
| `data_it.cpk::init.bin::insect/insect.bin::weapon.track` | 96 | `ee9843246fcfb03b6838114662756bbf7ef512cad1a1f7696fe017da4d6ba8e2` |
| `data_it.cpk::init_free.bin::system/effect/effpack.bin` | 208540 | `5d044b2b1060dd2caefdd1210fc093af274bd974d097f15732c865fcbd7c7b3b` |
| `data_k.cpk::battle/battle_pack.bin::BSV105.BVP` | 98944 | `83c524f15833266194779259d1fb2bf578f13e4ab67c6a7f5ef06d7ada83e6a9` |
| `data_k.cpk::battle/battle_pack2.bin::BSV108.BVP` | 94288 | `fa1415fcff9fdd938d641baee0986b5d316602aa74399834e4afcc3c538b4d58` |
| `data_k.cpk::battle/BSV105.BVP` | 98944 | `83c524f15833266194779259d1fb2bf578f13e4ab67c6a7f5ef06d7ada83e6a9` |
| `data_k.cpk::battle/BSV108.BVP` | 94288 | `fa1415fcff9fdd938d641baee0986b5d316602aa74399834e4afcc3c538b4d58` |
| `data_k.cpk::init.bin::insect/insect.bin::weapon.track` | 96 | `ee9843246fcfb03b6838114662756bbf7ef512cad1a1f7696fe017da4d6ba8e2` |
| `data_k.cpk::init_free.bin::system/effect/effpack.bin` | 208540 | `5d044b2b1060dd2caefdd1210fc093af274bd974d097f15732c865fcbd7c7b3b` |

The TRACK representative and BSV105 representative were re-extracted/read this session. Other hashes in this boundary annex are from the frozen M7c inventory, not independent re-extraction claims. effpack classification uses its existing recognized PAK-v2 children and22 positive TMX signatures. No game was executed.
