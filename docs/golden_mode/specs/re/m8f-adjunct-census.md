# M8f embedded P2A and TLP0 census

Companion to the complete leaf census. These are **embedded adjunct spans**, not additional top-level files. Traverse every one of the4,400 logical root objects, resolve each non-clone descriptor and nested EP/EPL, and count each physical descriptor address once within its root. Shared/direct references do not multiply the same physical descriptor. Locale copies remain separate logical occurrences. Hashes cover exactly the descriptor-declared adjunct span, including preserved trailers.

| CPK | P2A spans | TLP0 spans |
|---|---:|---:|
| data.cpk | 397 | 60 |
| data_c.cpk | 38 | 31 |
| data_ck.cpk | 37 | 7 |
| data_de.cpk | 37 | 7 |
| data_e.cpk | 41 | 31 |
| data_es.cpk | 37 | 7 |
| data_fr.cpk | 37 | 7 |
| data_it.cpk | 37 | 7 |
| data_k.cpk | 41 | 31 |

Totals: P2A702; TLP0188. Distinct SHA-256 identities: P2A43; TLP023.

## Per-immediate-parent archive counts

| Archive path | P2A | TLP0 |
|---|---:|---:|
| `data.cpk` | 7 | 2 |
| `data.cpk::battle/battle_pack.bin::btleff.bin` | 3 | 0 |
| `data.cpk::battle/battle_pack2.bin::btleff.bin` | 3 | 0 |
| `data.cpk::battle/btleff.bin` | 3 | 0 |
| `data.cpk::battle/result/shuffle.arc` | 4 | 0 |
| `data.cpk::event_data/community.bin` | 6 | 1 |
| `data.cpk::facility/cmbroot.arc` | 1 | 0 |
| `data.cpk::field/btl_effect/btl_eff.pac` | 3 | 0 |
| `data.cpk::field/fld_effect.bin` | 2 | 0 |
| `data.cpk::field/pack/dng_init.pac` | 3 | 0 |
| `data.cpk::field/pack/fd031_001.arc` | 2 | 1 |
| `data.cpk::field/pack/fd031_002.arc` | 2 | 1 |
| `data.cpk::field/pack/fd049_000.arc` | 2 | 1 |
| `data.cpk::field/pack/fd069_001.arc` | 2 | 1 |
| `data.cpk::field/pack/fd069_002.arc` | 2 | 1 |
| `data.cpk::field/pack/fd069_003.arc` | 2 | 1 |
| `data.cpk::field/pack/fd069_004.arc` | 2 | 1 |
| `data.cpk::init.bin` | 2 | 0 |
| `data.cpk::init.bin::event/icon.bin` | 6 | 0 |
| `data.cpk::skill/000.pac` | 1 | 0 |
| `data.cpk::skill/001.pac` | 1 | 1 |
| `data.cpk::skill/002.pac` | 1 | 0 |
| `data.cpk::skill/004.pac` | 1 | 1 |
| `data.cpk::skill/005.pac` | 1 | 0 |
| `data.cpk::skill/007.pac` | 1 | 1 |
| `data.cpk::skill/009.pac` | 1 | 0 |
| `data.cpk::skill/00A.pac` | 1 | 0 |
| `data.cpk::skill/010.pac` | 1 | 0 |
| `data.cpk::skill/013.pac` | 1 | 0 |
| `data.cpk::skill/015.pac` | 1 | 0 |
| `data.cpk::skill/016.pac` | 1 | 0 |
| `data.cpk::skill/018.pac` | 0 | 1 |
| `data.cpk::skill/01A.pac` | 1 | 0 |
| `data.cpk::skill/01D.pac` | 1 | 0 |
| `data.cpk::skill/021.pac` | 1 | 0 |
| `data.cpk::skill/022.pac` | 1 | 0 |
| `data.cpk::skill/024.pac` | 1 | 0 |
| `data.cpk::skill/025.pac` | 0 | 1 |
| `data.cpk::skill/026.pac` | 1 | 0 |
| `data.cpk::skill/027.pac` | 1 | 0 |
| `data.cpk::skill/028.pac` | 3 | 0 |
| `data.cpk::skill/029.pac` | 4 | 0 |
| `data.cpk::skill/02A.pac` | 1 | 0 |
| `data.cpk::skill/02B.pac` | 2 | 0 |
| `data.cpk::skill/02C.pac` | 3 | 0 |
| `data.cpk::skill/02D.pac` | 0 | 1 |
| `data.cpk::skill/02E.pac` | 4 | 0 |
| `data.cpk::skill/02F.pac` | 1 | 0 |
| `data.cpk::skill/030.pac` | 1 | 0 |
| `data.cpk::skill/031.pac` | 1 | 0 |
| `data.cpk::skill/037.pac` | 6 | 0 |
| `data.cpk::skill/038.pac` | 4 | 0 |
| `data.cpk::skill/039.pac` | 4 | 0 |
| `data.cpk::skill/03A.pac` | 3 | 0 |
| `data.cpk::skill/03B.pac` | 2 | 0 |
| `data.cpk::skill/03C.pac` | 1 | 0 |
| `data.cpk::skill/03F.pac` | 0 | 2 |
| `data.cpk::skill/040.pac` | 0 | 1 |
| `data.cpk::skill/041.pac` | 1 | 0 |
| `data.cpk::skill/043.pac` | 1 | 0 |
| `data.cpk::skill/044.pac` | 1 | 0 |
| `data.cpk::skill/045.pac` | 1 | 0 |
| `data.cpk::skill/047.pac` | 1 | 0 |
| `data.cpk::skill/048.pac` | 5 | 0 |
| `data.cpk::skill/049.pac` | 2 | 0 |
| `data.cpk::skill/04A.pac` | 3 | 0 |
| `data.cpk::skill/04B.pac` | 1 | 0 |
| `data.cpk::skill/04C.pac` | 1 | 0 |
| `data.cpk::skill/04D.pac` | 1 | 0 |
| `data.cpk::skill/04F.pac` | 1 | 0 |
| `data.cpk::skill/050.pac` | 1 | 0 |
| `data.cpk::skill/051.pac` | 1 | 0 |
| `data.cpk::skill/052.pac` | 1 | 0 |
| `data.cpk::skill/053.pac` | 2 | 0 |
| `data.cpk::skill/054.pac` | 1 | 0 |
| `data.cpk::skill/05C.pac` | 1 | 0 |
| `data.cpk::skill/05E.pac` | 1 | 0 |
| `data.cpk::skill/05F.pac` | 1 | 0 |
| `data.cpk::skill/060.pac` | 0 | 1 |
| `data.cpk::skill/069.pac` | 0 | 2 |
| `data.cpk::skill/06D.pac` | 1 | 0 |
| `data.cpk::skill/076.pac` | 1 | 0 |
| `data.cpk::skill/077.pac` | 1 | 0 |
| `data.cpk::skill/081.pac` | 1 | 0 |
| `data.cpk::skill/082.pac` | 1 | 0 |
| `data.cpk::skill/086.pac` | 1 | 0 |
| `data.cpk::skill/089.pac` | 1 | 0 |
| `data.cpk::skill/08D.pac` | 1 | 0 |
| `data.cpk::skill/08E.pac` | 2 | 0 |
| `data.cpk::skill/08F.pac` | 1 | 0 |
| `data.cpk::skill/092.pac` | 1 | 0 |
| `data.cpk::skill/098.pac` | 1 | 0 |
| `data.cpk::skill/09A.pac` | 1 | 0 |
| `data.cpk::skill/0A2.pac` | 1 | 0 |
| `data.cpk::skill/0A4.pac` | 1 | 0 |
| `data.cpk::skill/0A7.pac` | 1 | 0 |
| `data.cpk::skill/0AD.pac` | 1 | 0 |
| `data.cpk::skill/0C0.pac` | 1 | 0 |
| `data.cpk::skill/0C1.pac` | 1 | 0 |
| `data.cpk::skill/0C2.pac` | 1 | 0 |
| `data.cpk::skill/0C3.pac` | 1 | 0 |
| `data.cpk::skill/0C4.pac` | 1 | 0 |
| `data.cpk::skill/0C5.pac` | 1 | 0 |
| `data.cpk::skill/0C6.pac` | 1 | 0 |
| `data.cpk::skill/0C7.pac` | 1 | 0 |
| `data.cpk::skill/0C8.pac` | 1 | 0 |
| `data.cpk::skill/0C9.pac` | 1 | 0 |
| `data.cpk::skill/0CC.pac` | 1 | 0 |
| `data.cpk::skill/0CD.pac` | 1 | 0 |
| `data.cpk::skill/0CF.pac` | 1 | 0 |
| `data.cpk::skill/0D0.pac` | 1 | 0 |
| `data.cpk::skill/0D1.pac` | 1 | 0 |
| `data.cpk::skill/0D2.pac` | 1 | 0 |
| `data.cpk::skill/0D9.pac` | 2 | 0 |
| `data.cpk::skill/0E0.pac` | 2 | 0 |
| `data.cpk::skill/0E3.pac` | 1 | 0 |
| `data.cpk::skill/0E4.pac` | 1 | 0 |
| `data.cpk::skill/0EC.pac` | 1 | 0 |
| `data.cpk::skill/0ED.pac` | 1 | 0 |
| `data.cpk::skill/0EF.pac` | 1 | 0 |
| `data.cpk::skill/0F1.pac` | 4 | 0 |
| `data.cpk::skill/0F2.pac` | 3 | 0 |
| `data.cpk::skill/0F3.pac` | 2 | 0 |
| `data.cpk::skill/0F4.pac` | 1 | 0 |
| `data.cpk::skill/0F5.pac` | 1 | 0 |
| `data.cpk::skill/0F6.pac` | 1 | 0 |
| `data.cpk::skill/0FD.pac` | 1 | 0 |
| `data.cpk::skill/0FE.pac` | 1 | 0 |
| `data.cpk::skill/106.pac` | 1 | 0 |
| `data.cpk::skill/10B.pac` | 1 | 0 |
| `data.cpk::skill/10C.pac` | 1 | 0 |
| `data.cpk::skill/10D.pac` | 1 | 0 |
| `data.cpk::skill/10E.pac` | 1 | 0 |
| `data.cpk::skill/10F.pac` | 1 | 0 |
| `data.cpk::skill/110.pac` | 1 | 0 |
| `data.cpk::skill/112.pac` | 1 | 0 |
| `data.cpk::skill/113.pac` | 1 | 0 |
| `data.cpk::skill/114.pac` | 1 | 0 |
| `data.cpk::skill/115.pac` | 1 | 0 |
| `data.cpk::skill/118.pac` | 1 | 0 |
| `data.cpk::skill/11D.pac` | 2 | 0 |
| `data.cpk::skill/11E.pac` | 1 | 0 |
| `data.cpk::skill/11F.pac` | 1 | 0 |
| `data.cpk::skill/120.pac` | 1 | 0 |
| `data.cpk::skill/121.pac` | 1 | 0 |
| `data.cpk::skill/122.pac` | 1 | 0 |
| `data.cpk::skill/123.pac` | 1 | 0 |
| `data.cpk::skill/124.pac` | 1 | 0 |
| `data.cpk::skill/125.pac` | 1 | 0 |
| `data.cpk::skill/126.pac` | 1 | 0 |
| `data.cpk::skill/127.pac` | 1 | 0 |
| `data.cpk::skill/128.pac` | 1 | 0 |
| `data.cpk::skill/129.pac` | 1 | 0 |
| `data.cpk::skill/12A.pac` | 1 | 0 |
| `data.cpk::skill/12B.pac` | 1 | 0 |
| `data.cpk::skill/12C.pac` | 1 | 0 |
| `data.cpk::skill/12D.pac` | 1 | 0 |
| `data.cpk::skill/12E.pac` | 1 | 0 |
| `data.cpk::skill/12F.pac` | 1 | 0 |
| `data.cpk::skill/133.pac` | 1 | 0 |
| `data.cpk::skill/134.pac` | 1 | 0 |
| `data.cpk::skill/135.pac` | 1 | 0 |
| `data.cpk::skill/136.pac` | 1 | 0 |
| `data.cpk::skill/137.pac` | 1 | 0 |
| `data.cpk::skill/138.pac` | 1 | 0 |
| `data.cpk::skill/139.pac` | 1 | 0 |
| `data.cpk::skill/13A.pac` | 1 | 0 |
| `data.cpk::skill/13B.pac` | 1 | 0 |
| `data.cpk::skill/13C.pac` | 1 | 0 |
| `data.cpk::skill/13D.pac` | 1 | 0 |
| `data.cpk::skill/143.pac` | 2 | 0 |
| `data.cpk::skill/144.pac` | 2 | 0 |
| `data.cpk::skill/145.pac` | 1 | 0 |
| `data.cpk::skill/146.pac` | 1 | 1 |
| `data.cpk::skill/148.pac` | 1 | 0 |
| `data.cpk::skill/14B.pac` | 3 | 0 |
| `data.cpk::skill/14C.pac` | 1 | 0 |
| `data.cpk::skill/14F.pac` | 2 | 0 |
| `data.cpk::skill/151.pac` | 1 | 0 |
| `data.cpk::skill/152.pac` | 1 | 0 |
| `data.cpk::skill/153.pac` | 4 | 0 |
| `data.cpk::skill/154.pac` | 1 | 0 |
| `data.cpk::skill/155.pac` | 1 | 0 |
| `data.cpk::skill/156.pac` | 1 | 0 |
| `data.cpk::skill/157.pac` | 1 | 0 |
| `data.cpk::skill/158.pac` | 1 | 0 |
| `data.cpk::skill/159.pac` | 1 | 0 |
| `data.cpk::skill/15A.pac` | 1 | 0 |
| `data.cpk::skill/15B.pac` | 1 | 0 |
| `data.cpk::skill/15C.pac` | 1 | 0 |
| `data.cpk::skill/15D.pac` | 1 | 0 |
| `data.cpk::skill/15E.pac` | 1 | 0 |
| `data.cpk::skill/15F.pac` | 1 | 0 |
| `data.cpk::skill/162.pac` | 1 | 0 |
| `data.cpk::skill/168.pac` | 1 | 0 |
| `data.cpk::skill/172.pac` | 0 | 3 |
| `data.cpk::skill/173.pac` | 0 | 5 |
| `data.cpk::skill/174.pac` | 0 | 4 |
| `data.cpk::skill/175.pac` | 0 | 4 |
| `data.cpk::skill/176.pac` | 0 | 4 |
| `data.cpk::skill/177.pac` | 0 | 4 |
| `data.cpk::skill/179.pac` | 0 | 4 |
| `data.cpk::skill/17B.pac` | 1 | 0 |
| `data.cpk::skill/17C.pac` | 3 | 0 |
| `data.cpk::skill/17D.pac` | 0 | 7 |
| `data.cpk::skill/184.pac` | 1 | 0 |
| `data.cpk::skill/188.pac` | 0 | 1 |
| `data.cpk::skill/189.pac` | 0 | 1 |
| `data.cpk::skill/18C.pac` | 1 | 0 |
| `data.cpk::skill/18D.pac` | 1 | 0 |
| `data.cpk::skill/190.pac` | 1 | 0 |
| `data.cpk::skill/191.pac` | 1 | 0 |
| `data.cpk::skill/192.pac` | 1 | 0 |
| `data.cpk::skill/193.pac` | 1 | 0 |
| `data.cpk::skill/194.pac` | 1 | 0 |
| `data.cpk::skill/195.pac` | 1 | 0 |
| `data.cpk::skill/196.pac` | 1 | 0 |
| `data.cpk::skill/197.pac` | 1 | 0 |
| `data.cpk::skill/198.pac` | 1 | 0 |
| `data.cpk::skill/199.pac` | 1 | 0 |
| `data.cpk::skill/19A.pac` | 1 | 0 |
| `data.cpk::skill/19B.pac` | 1 | 0 |
| `data.cpk::skill/19C.pac` | 1 | 0 |
| `data.cpk::skill/19D.pac` | 1 | 0 |
| `data.cpk::skill/19E.pac` | 1 | 0 |
| `data.cpk::skill/19F.pac` | 1 | 0 |
| `data.cpk::skill/1A0.pac` | 1 | 0 |
| `data.cpk::skill/1A1.pac` | 1 | 0 |
| `data.cpk::skill/1A2.pac` | 1 | 0 |
| `data.cpk::skill/1A3.pac` | 1 | 0 |
| `data.cpk::skill/1A4.pac` | 1 | 0 |
| `data.cpk::skill/1A5.pac` | 1 | 0 |
| `data.cpk::skill/1A6.pac` | 1 | 0 |
| `data.cpk::skill/1A7.pac` | 1 | 0 |
| `data.cpk::skill/1A8.pac` | 1 | 0 |
| `data.cpk::skill/1A9.pac` | 1 | 0 |
| `data.cpk::skill/1AA.pac` | 1 | 0 |
| `data.cpk::skill/1AB.pac` | 1 | 0 |
| `data.cpk::skill/1AC.pac` | 1 | 0 |
| `data.cpk::skill/1AD.pac` | 1 | 0 |
| `data.cpk::skill/1AE.pac` | 1 | 0 |
| `data.cpk::skill/1AF.pac` | 1 | 0 |
| `data.cpk::skill/1B0.pac` | 1 | 0 |
| `data.cpk::skill/1B1.pac` | 1 | 0 |
| `data.cpk::skill/1B2.pac` | 1 | 0 |
| `data.cpk::skill/1B3.pac` | 1 | 0 |
| `data.cpk::skill/1B4.pac` | 1 | 0 |
| `data.cpk::skill/1B5.pac` | 1 | 0 |
| `data.cpk::skill/1B6.pac` | 1 | 0 |
| `data.cpk::skill/1B7.pac` | 1 | 0 |
| `data.cpk::skill/1B8.pac` | 1 | 0 |
| `data.cpk::skill/1B9.pac` | 1 | 0 |
| `data.cpk::skill/1BA.pac` | 1 | 0 |
| `data.cpk::skill/1BB.pac` | 1 | 0 |
| `data.cpk::skill/1BC.pac` | 1 | 0 |
| `data.cpk::skill/1BD.pac` | 1 | 0 |
| `data.cpk::skill/1C0.pac` | 3 | 0 |
| `data.cpk::skill/1C2.pac` | 1 | 0 |
| `data.cpk::skill/1C3.pac` | 1 | 0 |
| `data.cpk::skill/1C4.pac` | 4 | 0 |
| `data.cpk::skill/1C5.pac` | 1 | 0 |
| `data.cpk::skill/1C6.pac` | 1 | 0 |
| `data.cpk::skill/1C7.pac` | 1 | 0 |
| `data.cpk::skill/1C8.pac` | 1 | 0 |
| `data.cpk::skill/1C9.pac` | 1 | 0 |
| `data.cpk::skill/1CA.pac` | 1 | 0 |
| `data.cpk::skill/1CB.pac` | 1 | 0 |
| `data.cpk::skill/1CC.pac` | 1 | 0 |
| `data.cpk::skill/1CD.pac` | 1 | 0 |
| `data.cpk::skill/1CE.pac` | 1 | 0 |
| `data.cpk::skill/1CF.pac` | 1 | 0 |
| `data.cpk::skill/1D0.pac` | 1 | 0 |
| `data.cpk::skill/1D1.pac` | 1 | 0 |
| `data.cpk::skill/1D2.pac` | 1 | 0 |
| `data.cpk::skill/1D3.pac` | 1 | 0 |
| `data.cpk::skill/1D4.pac` | 1 | 0 |
| `data.cpk::skill/1D5.pac` | 1 | 0 |
| `data.cpk::skill/1D6.pac` | 1 | 0 |
| `data.cpk::skill/1D7.pac` | 1 | 0 |
| `data.cpk::skill/1D8.pac` | 1 | 0 |
| `data.cpk::skill/1D9.pac` | 1 | 0 |
| `data.cpk::skill/1DA.pac` | 1 | 0 |
| `data.cpk::skill/1DB.pac` | 1 | 0 |
| `data.cpk::skill/1DC.pac` | 1 | 0 |
| `data.cpk::skill/1DD.pac` | 1 | 0 |
| `data.cpk::skill/1DE.pac` | 1 | 0 |
| `data.cpk::skill/1DF.pac` | 1 | 0 |
| `data.cpk::skill/1E0.pac` | 1 | 0 |
| `data.cpk::skill/1E1.pac` | 1 | 0 |
| `data.cpk::skill/1E2.pac` | 1 | 0 |
| `data.cpk::skill/1E3.pac` | 1 | 0 |
| `data.cpk::skill/1E4.pac` | 1 | 0 |
| `data.cpk::skill/1E5.pac` | 1 | 0 |
| `data.cpk::skill/1E6.pac` | 1 | 0 |
| `data.cpk::skill/1E7.pac` | 1 | 0 |
| `data.cpk::skill/1E8.pac` | 1 | 0 |
| `data.cpk::skill/1E9.pac` | 1 | 0 |
| `data.cpk::skill/1EA.pac` | 1 | 0 |
| `data.cpk::skill/1EB.pac` | 1 | 0 |
| `data.cpk::skill/1EC.pac` | 1 | 0 |
| `data.cpk::skill/1ED.pac` | 1 | 0 |
| `data.cpk::skill/1EE.pac` | 1 | 0 |
| `data.cpk::skill/1EF.pac` | 1 | 0 |
| `data.cpk::skill/1F0.pac` | 1 | 0 |
| `data.cpk::skill/1F1.pac` | 1 | 0 |
| `data.cpk::skill/1F2.pac` | 1 | 0 |
| `data.cpk::skill/1F3.pac` | 1 | 0 |
| `data.cpk::skill/1F4.pac` | 1 | 0 |
| `data.cpk::skill/1F5.pac` | 1 | 0 |
| `data.cpk::skill/1F6.pac` | 1 | 0 |
| `data.cpk::skill/1F7.pac` | 1 | 0 |
| `data.cpk::skill/1F8.pac` | 1 | 0 |
| `data.cpk::skill/1F9.pac` | 1 | 0 |
| `data.cpk::skill/1FA.pac` | 1 | 0 |
| `data.cpk::skill/1FB.pac` | 1 | 0 |
| `data.cpk::skill/1FC.pac` | 1 | 0 |
| `data.cpk::skill/1FD.pac` | 1 | 0 |
| `data.cpk::skill/1FE.pac` | 1 | 0 |
| `data_c.cpk` | 1 | 0 |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin` | 3 | 0 |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin` | 3 | 0 |
| `data_c.cpk::battle/result/shuffle.arc` | 4 | 0 |
| `data_c.cpk::event_data/community.bin` | 6 | 1 |
| `data_c.cpk::facility/cmbroot.arc` | 1 | 0 |
| `data_c.cpk::field/pack/fd031_001.arc` | 2 | 1 |
| `data_c.cpk::field/pack/fd031_002.arc` | 2 | 1 |
| `data_c.cpk::field/pack/fd069_001.arc` | 2 | 1 |
| `data_c.cpk::field/pack/fd069_002.arc` | 2 | 1 |
| `data_c.cpk::field/pack/fd069_003.arc` | 2 | 1 |
| `data_c.cpk::field/pack/fd069_004.arc` | 2 | 1 |
| `data_c.cpk::init.bin` | 2 | 0 |
| `data_c.cpk::init.bin::event/icon.bin` | 6 | 0 |
| `data_c.cpk::skill/172.pac` | 0 | 3 |
| `data_c.cpk::skill/173.pac` | 0 | 5 |
| `data_c.cpk::skill/174.pac` | 0 | 4 |
| `data_c.cpk::skill/175.pac` | 0 | 4 |
| `data_c.cpk::skill/176.pac` | 0 | 4 |
| `data_c.cpk::skill/177.pac` | 0 | 4 |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin` | 3 | 0 |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin` | 3 | 0 |
| `data_ck.cpk::battle/result/shuffle.arc` | 4 | 0 |
| `data_ck.cpk::event_data/community.bin` | 6 | 1 |
| `data_ck.cpk::facility/cmbroot.arc` | 1 | 0 |
| `data_ck.cpk::field/pack/fd031_001.arc` | 2 | 1 |
| `data_ck.cpk::field/pack/fd031_002.arc` | 2 | 1 |
| `data_ck.cpk::field/pack/fd069_001.arc` | 2 | 1 |
| `data_ck.cpk::field/pack/fd069_002.arc` | 2 | 1 |
| `data_ck.cpk::field/pack/fd069_003.arc` | 2 | 1 |
| `data_ck.cpk::field/pack/fd069_004.arc` | 2 | 1 |
| `data_ck.cpk::init.bin` | 2 | 0 |
| `data_ck.cpk::init.bin::event/icon.bin` | 6 | 0 |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin` | 3 | 0 |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin` | 3 | 0 |
| `data_de.cpk::battle/result/shuffle.arc` | 4 | 0 |
| `data_de.cpk::event_data/community.bin` | 6 | 1 |
| `data_de.cpk::facility/cmbroot.arc` | 1 | 0 |
| `data_de.cpk::field/pack/fd031_001.arc` | 2 | 1 |
| `data_de.cpk::field/pack/fd031_002.arc` | 2 | 1 |
| `data_de.cpk::field/pack/fd069_001.arc` | 2 | 1 |
| `data_de.cpk::field/pack/fd069_002.arc` | 2 | 1 |
| `data_de.cpk::field/pack/fd069_003.arc` | 2 | 1 |
| `data_de.cpk::field/pack/fd069_004.arc` | 2 | 1 |
| `data_de.cpk::init.bin` | 2 | 0 |
| `data_de.cpk::init.bin::event/icon.bin` | 6 | 0 |
| `data_e.cpk` | 1 | 0 |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin` | 3 | 0 |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin` | 3 | 0 |
| `data_e.cpk::battle/btleff.bin` | 3 | 0 |
| `data_e.cpk::battle/result/shuffle.arc` | 4 | 0 |
| `data_e.cpk::event_data/community.bin` | 6 | 1 |
| `data_e.cpk::facility/cmbroot.arc` | 1 | 0 |
| `data_e.cpk::field/pack/fd031_001.arc` | 2 | 1 |
| `data_e.cpk::field/pack/fd031_002.arc` | 2 | 1 |
| `data_e.cpk::field/pack/fd069_001.arc` | 2 | 1 |
| `data_e.cpk::field/pack/fd069_002.arc` | 2 | 1 |
| `data_e.cpk::field/pack/fd069_003.arc` | 2 | 1 |
| `data_e.cpk::field/pack/fd069_004.arc` | 2 | 1 |
| `data_e.cpk::init.bin` | 2 | 0 |
| `data_e.cpk::init.bin::event/icon.bin` | 6 | 0 |
| `data_e.cpk::skill/172.pac` | 0 | 3 |
| `data_e.cpk::skill/173.pac` | 0 | 5 |
| `data_e.cpk::skill/174.pac` | 0 | 4 |
| `data_e.cpk::skill/175.pac` | 0 | 4 |
| `data_e.cpk::skill/176.pac` | 0 | 4 |
| `data_e.cpk::skill/177.pac` | 0 | 4 |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin` | 3 | 0 |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin` | 3 | 0 |
| `data_es.cpk::battle/result/shuffle.arc` | 4 | 0 |
| `data_es.cpk::event_data/community.bin` | 6 | 1 |
| `data_es.cpk::facility/cmbroot.arc` | 1 | 0 |
| `data_es.cpk::field/pack/fd031_001.arc` | 2 | 1 |
| `data_es.cpk::field/pack/fd031_002.arc` | 2 | 1 |
| `data_es.cpk::field/pack/fd069_001.arc` | 2 | 1 |
| `data_es.cpk::field/pack/fd069_002.arc` | 2 | 1 |
| `data_es.cpk::field/pack/fd069_003.arc` | 2 | 1 |
| `data_es.cpk::field/pack/fd069_004.arc` | 2 | 1 |
| `data_es.cpk::init.bin` | 2 | 0 |
| `data_es.cpk::init.bin::event/icon.bin` | 6 | 0 |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin` | 3 | 0 |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin` | 3 | 0 |
| `data_fr.cpk::battle/result/shuffle.arc` | 4 | 0 |
| `data_fr.cpk::event_data/community.bin` | 6 | 1 |
| `data_fr.cpk::facility/cmbroot.arc` | 1 | 0 |
| `data_fr.cpk::field/pack/fd031_001.arc` | 2 | 1 |
| `data_fr.cpk::field/pack/fd031_002.arc` | 2 | 1 |
| `data_fr.cpk::field/pack/fd069_001.arc` | 2 | 1 |
| `data_fr.cpk::field/pack/fd069_002.arc` | 2 | 1 |
| `data_fr.cpk::field/pack/fd069_003.arc` | 2 | 1 |
| `data_fr.cpk::field/pack/fd069_004.arc` | 2 | 1 |
| `data_fr.cpk::init.bin` | 2 | 0 |
| `data_fr.cpk::init.bin::event/icon.bin` | 6 | 0 |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin` | 3 | 0 |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin` | 3 | 0 |
| `data_it.cpk::battle/result/shuffle.arc` | 4 | 0 |
| `data_it.cpk::event_data/community.bin` | 6 | 1 |
| `data_it.cpk::facility/cmbroot.arc` | 1 | 0 |
| `data_it.cpk::field/pack/fd031_001.arc` | 2 | 1 |
| `data_it.cpk::field/pack/fd031_002.arc` | 2 | 1 |
| `data_it.cpk::field/pack/fd069_001.arc` | 2 | 1 |
| `data_it.cpk::field/pack/fd069_002.arc` | 2 | 1 |
| `data_it.cpk::field/pack/fd069_003.arc` | 2 | 1 |
| `data_it.cpk::field/pack/fd069_004.arc` | 2 | 1 |
| `data_it.cpk::init.bin` | 2 | 0 |
| `data_it.cpk::init.bin::event/icon.bin` | 6 | 0 |
| `data_k.cpk` | 1 | 0 |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin` | 3 | 0 |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin` | 3 | 0 |
| `data_k.cpk::battle/btleff.bin` | 3 | 0 |
| `data_k.cpk::battle/result/shuffle.arc` | 4 | 0 |
| `data_k.cpk::event_data/community.bin` | 6 | 1 |
| `data_k.cpk::facility/cmbroot.arc` | 1 | 0 |
| `data_k.cpk::field/pack/fd031_001.arc` | 2 | 1 |
| `data_k.cpk::field/pack/fd031_002.arc` | 2 | 1 |
| `data_k.cpk::field/pack/fd069_001.arc` | 2 | 1 |
| `data_k.cpk::field/pack/fd069_002.arc` | 2 | 1 |
| `data_k.cpk::field/pack/fd069_003.arc` | 2 | 1 |
| `data_k.cpk::field/pack/fd069_004.arc` | 2 | 1 |
| `data_k.cpk::init.bin` | 2 | 0 |
| `data_k.cpk::init.bin::event/icon.bin` | 6 | 0 |
| `data_k.cpk::skill/172.pac` | 0 | 3 |
| `data_k.cpk::skill/173.pac` | 0 | 5 |
| `data_k.cpk::skill/174.pac` | 0 | 4 |
| `data_k.cpk::skill/175.pac` | 0 | 4 |
| `data_k.cpk::skill/176.pac` | 0 | 4 |
| `data_k.cpk::skill/177.pac` | 0 | 4 |

## Identity hashes

The example location is a complete inventory path plus root-relative byte offset (hex). The occurrence count includes all CPKs; per-parent counts above retain archive provenance.

| Family | Bytes | SHA-256 | Occurrences | First example and offset |
|---|---:|---|---:|---|
| P2A | 5280 | `056f78c2a98bf3fc274e89b0e8934af50a1791a18284b089b08ad5125494c514` | 18 | `data.cpk::skill/039.pac::039.BED` +`0x16A0` |
| P2A | 34048 | `086a3a12270c4359fa47951da4999cf464103a0143105f22e888dba358977cd8` | 9 | `data.cpk::init.bin::event/icon.bin::_ICON9.EPL` +`0x270` |
| P2A | 17568 | `08f8d7a26f6bc4faba1fa536e71ee8fe4af0ffba7c04b661c4d052ee3e2052e5` | 2 | `data.cpk::skill/0E3.pac::0E3.BED` +`0x6050` |
| P2A | 17568 | `10774eb37410820d9eda2355e55628b41d2a6ec107ae0ee65e0de7b5e9951151` | 1 | `data.cpk::skill/05C.pac::05C.BED` +`0xC250` |
| P2A | 2272 | `1d39444bc3e695774cd56738a8e649d11372c1d4942761213246b55225da5d6a` | 2 | `data.cpk::skill/045.pac::045.BED` +`0x14280` |
| P2A | 17856 | `20498c364f6219b7504bdcb735b902ea919885bde24110f53ef944926a6b79ca` | 3 | `data.cpk::skill/044.pac::044.BED` +`0xF730` |
| P2A | 8416 | `2141e4697e608f9d86b182c1e75b4a648263922be2994a0b3c80623165313a2f` | 3 | `data.cpk::field/TRAEST.EPL` +`0xA9E0` |
| P2A | 17568 | `2479360cb2ad810ab3d285587ea821db4fe87fefdf91f5714de00dca9db7b080` | 1 | `data.cpk::skill/17C.pac::17C.BED` +`0x11960` |
| P2A | 2272 | `3ef912302edae1d9872b95baedeb062c7c3db1b6be3b8268648435503f5050a4` | 45 | `data.cpk::init.bin::event/icon.bin::_ICON12.EPL` +`0x570` |
| P2A | 5280 | `4272575569843a82f6ffe0797534c10224d9f94ceaa73546dd0c5e4063ebd634` | 1 | `data.cpk::skill/162.pac::162.BED` +`0x29E0` |
| P2A | 8992 | `4554c50046298c0af3c7b515d2ba957f12713c14d50ec97d9dbed9a2a5d7dc52` | 15 | `data.cpk::skill/037.pac::037.BED` +`0x131A0` |
| P2A | 5280 | `505e77c4979df5a53526f952dce1fad19fa5f8fa9b8fe839ec67cb8c03b0a6ca` | 9 | `data.cpk::skill/027.pac::027.BED` +`0x1990` |
| P2A | 8560 | `559d1239898007c81aea009006c8b1936afa03e29fa76ee8b929e349ca9525b3` | 3 | `data.cpk::battle/bike/B118_W.BED` +`0x9DE0` |
| P2A | 66976 | `6382da5e677b84afb91c33e6e2179016eb4026dcc05f7819ec0491a7e77ddc01` | 55 | `data.cpk::field/pack/fd031_001.arc::F_SNOW.EPL` +`0x270` |
| P2A | 5280 | `71324b41677577227802991c6cb217d38f1b8f487ee0760c87ee047012bac561` | 22 | `data.cpk::battle/battle_pack.bin::btleff.bin::KIZETU.BED` +`0x8A0` |
| P2A | 5280 | `742c5c7aac51247e5ac3a6f241a9488f61f053edd61a29e45fcb260d135bcb69` | 5 | `data.cpk::skill/028.pac::028.BED` +`0x2370` |
| P2A | 2272 | `74d1cbc1bb959653e80686919e8ab8cf878c8a6e1951a77fc9e9a9662377e228` | 22 | `data.cpk::battle/battle_pack.bin::btleff.bin::PANIC.BED` +`0x8A0` |
| P2A | 33440 | `77dd5f6ced8ed51a425abedf1f9bb929b75230528099bb806a8997b3208f144c` | 2 | `data.cpk::field/btl_effect/btl_eff.pac::F_HAKEN.EPL` +`0x270` |
| P2A | 67216 | `848c8bc1f02a11b4eb79cf56445532df153503585eeafb1fbf20e71adc50dd73` | 1 | `data.cpk::skill/08D.pac::08D.BED` +`0x5C20` |
| P2A | 17568 | `8633ee10c05c20e9dda543a91b6fa45c329c4e4291f0e74ad9e6e7486194f796` | 46 | `data.cpk::skill/00A.pac::00A.BED` +`0x5B90` |
| P2A | 33424 | `894c9362faef1c369f37d25ae64d47bdffa1089a88074ee6e60ccb6e8aca6ea7` | 1 | `data.cpk::field/fld_effect.bin::F_HAKEN.EPL` +`0x270` |
| P2A | 17568 | `99e656e42327e726e1fa6fc550695b49873b06e2e68ddc621693319456ddcaa2` | 9 | `data.cpk::facility/cmbroot.arc::GAT07_01.EPL` +`0x740` |
| P2A | 8608 | `9f2743282f2637893c074283a1533197308c543e29940d380aa117147dcaa6b2` | 226 | `data.cpk::field/pack/fd031_001.arc::MAGATAMA.EPL` +`0x760` |
| P2A | 2528 | `a4690df2435a82789f676895665f908d03c6cbc87255d782246b39a9f4179c05` | 23 | `data.cpk::battle/battle_pack.bin::btleff.bin::GAL_CRT.BED` +`0x8A0` |
| P2A | 5280 | `a46d70c374d42b8fbc8a64783bfda999e45d971d3a385b1a6d0b5611d114273c` | 27 | `data.cpk::battle/result/shuffle.arc::SHUFF_01.EPL` +`0x7A0` |
| P2A | 5376 | `a5156c160b6c80c633b8d04958af935a5b0a03e2e294273d3dfa4184fadcb8f7` | 13 | `data.cpk::battle/union/U15E_A.BED` +`0x8320` |
| P2A | 5280 | `a53e95a5613df80a1399b649ef6055b6d08586031260ffb24fdbddd8df746f03` | 9 | `data.cpk::battle/result/shuffle.arc::SHUFFLE01.EPL` +`0x320` |
| P2A | 67024 | `a6dc02dfbefc393ee00768600b73f7bcc6230c1985b253e688817d5fe056f268` | 1 | `data.cpk::field/fld_effect.bin::P_HAKEN.EPL` +`0x270` |
| P2A | 5344 | `a80c35eb3bd4c4bf7025e2441cfb101998bf276f987ca4851be022465336bb2b` | 3 | `data.cpk::skill/02E.pac::02E.BED` +`0xA6B0` |
| P2A | 5328 | `a80dc9b16bb46df944ee6f34d9a6ac92644b06ccdc3876b31ee45c882a7f3bce` | 1 | `data.cpk::skill/17C.pac::17C.BED` +`0x10370` |
| P2A | 17568 | `a9b785e45f6069f88fb046710b2445be93e992b81db39555bf9882a4f88ce0a2` | 1 | `data.cpk::battle/assist/ASSIST_ST.BED` +`0xAD0` |
| P2A | 5824 | `b24a418615508c1f7cf1625b86848ca209c45be9594738083c753b5b9b9c5ab6` | 9 | `data.cpk::init.bin::field/F_ICON03.EPL` +`0x17F0` |
| P2A | 5376 | `b612a6d6fccd13a6d4e8f20b969ef7e05a5d7ca30488d07caa54df8c4a213c00` | 1 | `data.cpk::skill/17C.pac::17C.BED` +`0x81D0` |
| P2A | 5728 | `c71e833483de162aef3780f36c5e062258e72c2fe2cc6e43d40d69ae4ae65beb` | 9 | `data.cpk::init.bin::field/F_ICON01.EPL` +`0x17F0` |
| P2A | 5280 | `c987efb9d755139e2af6d485d8202011082e227758f5b1f55ab47b694bdf95de` | 18 | `data.cpk::event_data/community.bin::E_KIRA.EPL` +`0x1C70` |
| P2A | 5280 | `ca414d30affa4eaf99f28b7463bbab03bd4e9add28160eb72fe0700430677ba0` | 12 | `data.cpk::field/btl_effect/btl_eff.pac::F_SAISEI.EPL` +`0xE80` |
| P2A | 66960 | `cc4c7e5f7f1bd09802729ee1ec83d04770ae28a82977b95938b6d130aa06ad01` | 6 | `data.cpk::skill/010.pac::010.BED` +`0x31B0` |
| P2A | 8464 | `cef70ed324b52b73c305c82e34bc33859934851498230a3ec46b5929c9287b28` | 1 | `data.cpk::skill/04D.pac::04D.BED` +`0x63F0` |
| P2A | 17568 | `d1285d8c355a9cf96bb977c4c9d9278197cfa06f3705575e79b85b49fdf15387` | 8 | `data.cpk::skill/02E.pac::02E.BED` +`0x6100` |
| P2A | 8416 | `d78e9bc070f60359ab34999fc684cac568aa95e887d1b1af0d54b982a6f58afa` | 10 | `data.cpk::skill/053.pac::053.BED` +`0x2310` |
| P2A | 6128 | `e2dd5956fe08c165cac671d82a065e071d79141704aa655563bcf78b495a58a6` | 2 | `data.cpk::field/btl_effect/btl_eff.pac::F_SAISEI.EPL` +`0x24F0` |
| P2A | 2512 | `f2c342496884bcb1c9ef1c1db06c2947eaf2b7de220dc1bb07b9093e71c0e620` | 11 | `data.cpk::skill/037.pac::037.BED` +`0x45B0` |
| P2A | 5280 | `f6902e4496f91a3364060b4b5c75ae1a2f630c6a909329a21310b246cfe2765d` | 36 | `data.cpk::event_data/community.bin::CMY01_02.EPL` +`0x5D0` |
| TLP0 | 11632 | `0b62820e21494f80cd4eb1cc0d6179781ecf0ae11ec3afafad045402c379952b` | 4 | `data.cpk::skill/172.pac::172.BED` +`0x16880` |
| TLP0 | 16688 | `1d57df94c7a4e62443868eaf99d756159af3ef7fda00452c12d8d71496525c73` | 16 | `data.cpk::skill/174.pac::174.BED` +`0x37AF0` |
| TLP0 | 266576 | `222f5747fae815ee7d04aeff7e7d2f27ff2303dde5bd1d4b5606a369fb0f0098` | 9 | `data.cpk::event_data/community.bin::CARD_30.EPL` +`0x220` |
| TLP0 | 1984 | `2f6d30247d22708e022686521e9e870092a76dba62fa6fc2778cfd432e14e72c` | 7 | `data.cpk::skill/17D.pac::17D.BED` +`0x8540` |
| TLP0 | 43856 | `331ddab9f90a3fe253ef42b1df1fceb6cf9eaf03e6b41685bad3ca3923c3a452` | 4 | `data.cpk::skill/173.pac::173.BED` +`0x10CC0` |
| TLP0 | 13168 | `397b9ebb8d6dd1ddffa3f8f9111a4185df2b097f0cee7d2d47d2975a165ebad2` | 4 | `data.cpk::skill/173.pac::173.BED` +`0x22090` |
| TLP0 | 19920 | `7e83bcb46826f7d6a36bf2a5dcb0a4f350608caa1352bd48e39628a177971e9b` | 4 | `data.cpk::skill/174.pac::174.BED` +`0x308F0` |
| TLP0 | 1984 | `82998dd106d096d84522526a59c011d6fe0be06bd73a2529549d1e0ee053486c` | 1 | `data.cpk::skill/179.pac::179.BED` +`0x5590` |
| TLP0 | 17728 | `83547311975b43a182c1f88c2c10eb7dec72099b14dba50868b2428d6c6f147c` | 3 | `data_c.cpk::skill/173.pac::173.BED` +`0xC6B0` |
| TLP0 | 17728 | `83f53d0fd988930403278efa118eb5bcf407dbc95b135d56574cc5977b10d4f2` | 1 | `data.cpk::skill/173.pac::173.BED` +`0xC6B0` |
| TLP0 | 41872 | `9e71ce87d3887f7d683f1c64c166554bdf5fd94aa05b3a6cb7dc5cdd71bde8ca` | 6 | `data.cpk::skill/172.pac::172.BED` +`0x14D0` |
| TLP0 | 87456 | `9fbb11e884aefae03018c73a123f72a7cbf104c90b6ea497328f62b502bda7c9` | 5 | `data.cpk::skill/018.pac::018.BED` +`0x2F10` |
| TLP0 | 67616 | `a1049692e3715426a68b78caf86e035ec2c48aa0a3439dd40b453b080bb66d8a` | 59 | `data.cpk::field/pack/fd031_001.arc::MAGATAMA.EPL` +`0x3560` |
| TLP0 | 15536 | `a2c25e2e83a475eb810631637bab721b4a711afadf090401946593ef825cd347` | 1 | `data.cpk::skill/172.pac::172.BED` +`0xB930` |
| TLP0 | 41872 | `ae99e785f54548cd19d3b22b76150876b1d1f9fa069017b59c2fbb7e056e53a8` | 18 | `data_c.cpk::skill/172.pac::172.BED` +`0x14D0` |
| TLP0 | 13168 | `b3a556f2684fcc516369622278d3db77dbd2479c5bca29c0a7df7d45c8fb2799` | 4 | `data.cpk::skill/173.pac::173.BED` +`0x1DA30` |
| TLP0 | 78816 | `bd0447963b8891e487cfa137491236b4d9da72a25fafb5833fd536bdb20a8fed` | 7 | `data.cpk::skill/03F.pac::03F.BED` +`0x2820` |
| TLP0 | 1984 | `c9282eb71a61766b446675829acd330b9c10a0937f31848a53f20519f94e6c64` | 2 | `data.cpk::skill/179.pac::179.BED` +`0x9750` |
| TLP0 | 104944 | `d73c66b8d69b1bfd767b3927da8752f49d699bf94414380d8eeb811a1b4a4b0f` | 1 | `data.cpk::skill/060.pac::060.BED` +`0x137D0` |
| TLP0 | 15536 | `ddf26ca442150df5e41b5f989d2a8fa83f63fb90d2f73b6c9a4812533adc333c` | 3 | `data_c.cpk::skill/172.pac::172.BED` +`0xB930` |
| TLP0 | 19920 | `e923b5ce22d6c4e1aa61dcb50f5880d0c23df8117af9cb6897bda6ea2058a6d7` | 12 | `data_c.cpk::skill/174.pac::174.BED` +`0x308F0` |
| TLP0 | 1984 | `eb428cc46bccb59b6e152e03e5ed877779dc5d1974258be809c2bc90fc53b0b5` | 1 | `data.cpk::skill/179.pac::179.BED` +`0x60E0` |
| TLP0 | 141728 | `f4e90186c7a0346a3d7c1d94833b675b673d13a8e8ad9b9d8918951ff260dc4b` | 16 | `data.cpk::skill/174.pac::174.BED` +`0x3A20` |

Source: final private `census.json`, SHA-256 `d92005cc0da2b3d1cf773eccc631347289853f58b29f024e405a689a08271747`. Wire consumers/grammar and texture binding are specified in [m8f-texture-bindings.md](m8f-texture-bindings.md). The4 HFLC registry files are separately counted and hashed there; they are global binding metadata, not P2A/TLP root files.
