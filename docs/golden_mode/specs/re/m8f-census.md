# M8f Steam effect census

Source: Steam build10836033; full recursive M7c inventory SHA-256 `150454e1b60d50ccdd58346b6da4e18e363860bf49bb22a679db379dee3771f8`.

Logical occurrences retain locale and nested-pack duplicates. This is a census of named BED/EPL/EP/PB objects, not a claim that every editor sample is reachable in battle. Embedded objects are measured separately in [grammar](m8f-effect-grammar.md). Every complete leaf SHA below is the extracted, decompressed byte identity. One representative of each distinct SHA was re-extracted and hash/size checked; duplicate paths reuse the frozen inventory identity, not a new per-path extraction claim.

Discovery: inspected all161154 inventory rows, all signature labels, battle-directory extensions and effect-directory extensions. BED/EPL alone missed EP and PB; the final selection includes all four extensions. No other .eff/.eft/.emt/.eps/.epb/.ptcl extension occurs. Textures, models, sound and script containers are dependencies, not silently counted as a new particle grammar.

## Archive counts and identities

| CPK | Archive SHA-256 (M7c) | BED400 | Dummy BED | EPL | EP | PB | Selected-set SHA-256 |
|---|---|---:|---:|---:|---:|---:|---|
| data.cpk | `b9e88940eb044e813aaf097af94ecd21b21091a98bf8835c6c1505cd24ae08fb` | 816 | 83 | 400 | 148 | 1 | `fe39d42fe5839c5cfb4272c2d96567937550395b9015f1aff4185d38662de7bd` |
| data_c.cpk | `fdcad9007e6af09b869f425668b2ea70cd278a95f270db08b87dd6acfe787a5a` | 194 | 83 | 94 | 0 | 0 | `43f02f4bade097a5bd2a019391fedcc2954a43669e5a06b8d27c8e622f36e260` |
| data_ck.cpk | `df438493e5b11a02c41896c6eea9724eb49e981d7c4521ef45fe31df2aae7fa8` | 179 | 83 | 85 | 0 | 0 | `5ccd966519f2a6d51749189c65cfb903e78f7d49d45ecee0de024b83c817a7c2` |
| data_de.cpk | `90af464a61b57fa8cc24a1586febc786e668d5948d1de90ef898131751e4809d` | 179 | 83 | 85 | 0 | 0 | `39e9a5047cc919fff0bb1b99b4a3f879cdb4e55a935b7fa5978f6eaae52a1afb` |
| data_e.cpk | `620741aca9601ae2d48a391b87b3064c2e778b8f47833a360430cf0e55eeb8c1` | 246 | 83 | 94 | 0 | 0 | `2b5800095acc86ecf3c9f5cc12dc9d332a835ee59b2975ffdbd8af626a1a1caa` |
| data_es.cpk | `192e2eb06c0c6a39c5e02660d469eefa6ac6c0e5b3045d876d06632e3a9365bf` | 179 | 83 | 85 | 0 | 0 | `6d52ce636454e0d61b510d3d74bff881c2cf97efac8622e7a6efbadf18a3e4ef` |
| data_fr.cpk | `ba83853ea52629f9e3582d4f8c6ebfe2b4d1673c65723b17e464fc412357ce1b` | 179 | 83 | 85 | 0 | 0 | `699b082218d19bda95f76e40ee9f7af147cddf93705fd6b1ee3751ea39c19671` |
| data_it.cpk | `d9541ff225559857c0f5b65310e8fc560a400b5ffc3fad1023943f6fb29b52ac` | 179 | 83 | 85 | 0 | 0 | `3edf026f16ac33151c888f9688d8ed906580805edd17a301433d67fac6ba89eb` |
| data_k.cpk | `15956365ab57abad1251b74c2907b2d10f894baad2829cfce6ad0ef741893d9b` | 246 | 83 | 94 | 0 | 0 | `38c0882ed307e225a6ffa01144a66008749784848c8ee65b1860cd8d8c96068a` |
| data_movie.cpk | `ee1d1fb7a708dac75307e9edd561d13e525ff52b8593a5c737e17d8e82c8948c` | 0 | 0 | 0 | 0 | 0 | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| sysdat.cpk | `01f97f161b4a073fdcfa46b850d65365fbcebb6f57ec1fb1d9096736478b4f18` | 0 | 0 | 0 | 0 | 0 | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |

Selected-set digest: UTF-8, records sorted by full case-sensitive path, `path<TAB>decimal_size<TAB>lowercase_sha256<LF>`; empty archives use SHA-256 of empty bytes. Archive hashes are the pinned M7c inventory receipts; this lane did not re-hash every complete CPK.

Total: **4400 logical objects; 881 distinct extracted SHA-256 identities**. Family totals: BED400=2397, EP=148, EPL=1107, PB=1, dummy-BED=747.

## Per-archive member identities

The full path identifies every enclosing archive as well as the CPK. No basename-only deduplication is permitted. Parent-container counts and hashes follow the leaf table.

| Full logical path | Family | Bytes | Extracted SHA-256 |
|---|---|---:|---|
| `data.cpk::battle/assist/ANALYZE.BED` | BED400 | 94976 | `69d0bd59a2196ef02335dcc731617df0a4fa2f374e8e4e891fce622e7cfd22aa` |
| `data.cpk::battle/assist/ASSIST_EF.BED` | BED400 | 131408 | `186a53f1d731987312aacc5ca3463e198d750b5f0db2d1dddf931466a7673803` |
| `data.cpk::battle/assist/ASSIST_EF_S.BED` | BED400 | 17792 | `c568b47df3880ecbea1b2f36ae91421720851efb789d5c063966a0171a9ae86a` |
| `data.cpk::battle/assist/ASSIST_P000.BED` | BED400 | 228416 | `cbdede7738c6564add89c17eb5881105e5a5bae6976c231261701d3be1223b08` |
| `data.cpk::battle/assist/ASSIST_P001.BED` | BED400 | 329056 | `a5fa1476850d62362e0dce7862d0baf29857f7914cb18cd7b224d3d5ea074a39` |
| `data.cpk::battle/assist/ASSIST_P002.BED` | BED400 | 315040 | `6d9f5c55891f770397a6559b2ab01ebb9daa0bc393843aba0739b2c305beb302` |
| `data.cpk::battle/assist/ASSIST_RISE_S.BED` | BED400 | 257520 | `426137d979338c84a29266f7650e2302edd1c8f3112c5d99db94ae72c036ffaa` |
| `data.cpk::battle/assist/ASSIST_RISE_W.BED` | BED400 | 249216 | `11e984c53f57fe15b0de5c411ec35013199c0f8dc12158d6ab6eedfeebb21833` |
| `data.cpk::battle/assist/ASSIST_ST.BED` | BED400 | 25072 | `678b81263c15c4048ceea494a8ecc473b7b609f16a18bcff5c2d50fb9fad66fb` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::ATC_D_J.BED` | BED400 | 4448 | `f9ffa0abff120b243b99b3d6006697becb10735fc24f0fb798beab2a467ece7a` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::ATC_J_J.BED` | BED400 | 3616 | `b70ab3abd0d5491ba55fc2cb9ddfc918225ba0090e58e3aed9d396e628ce6dd3` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 6560 | `fa971120bb56cc962a0792d7af2a572779c23fa18769b814c5a531dc7e09f97f` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data.cpk::battle/battle_pack.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D_J.BED` | BED400 | 4448 | `f9ffa0abff120b243b99b3d6006697becb10735fc24f0fb798beab2a467ece7a` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J_J.BED` | BED400 | 3616 | `b70ab3abd0d5491ba55fc2cb9ddfc918225ba0090e58e3aed9d396e628ce6dd3` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 6560 | `fa971120bb56cc962a0792d7af2a572779c23fa18769b814c5a531dc7e09f97f` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data.cpk::battle/bike/B116_S.BED` | BED400 | 390704 | `8e45fa0e7473950136f3df80d0435ea89aa39e62ea21508cbebb5aa2adb1ad2e` |
| `data.cpk::battle/bike/B116_W.BED` | BED400 | 416848 | `53ed2407945efa17576143cd5a0d871d01d27e26f11bca6829d538eb3b0b9b84` |
| `data.cpk::battle/bike/B117_S.BED` | BED400 | 393792 | `3105d1a9b37a9e47834e2e038c0036e53d7c5b08eefdfd6ea718fce52ad1375d` |
| `data.cpk::battle/bike/B117_W.BED` | BED400 | 418832 | `20b41b984760a9dbc34bd730b0ce3123cb3733307ddb03935e240d7392f602c2` |
| `data.cpk::battle/bike/B118_S.BED` | BED400 | 348464 | `2313a7f8038e466259c78c21a6e14d487710f424eb0a8a4e8da13b76ee67cd54` |
| `data.cpk::battle/bike/B118_W.BED` | BED400 | 438944 | `7fd0c67f33cbcf875e9781c548b03b428312f270c95fa0d95fe928e860b11d81` |
| `data.cpk::battle/bike/B119_S.BED` | BED400 | 471152 | `b8765622faa144d650cbadc9ad734c6cca9037bbacad1161707fef608c315d18` |
| `data.cpk::battle/bike/B119_W.BED` | BED400 | 506576 | `4e55df3e0e557753143f58c847bff89c9f1ab932f3fa0463d2216ac6670ce98e` |
| `data.cpk::battle/bike/B11A_S.BED` | BED400 | 480400 | `660ad3616d1bbd5cf605ea34d5b3e877ed54a26482c0dc5e0ac002daa3a01163` |
| `data.cpk::battle/bike/B11A_W.BED` | BED400 | 480400 | `660ad3616d1bbd5cf605ea34d5b3e877ed54a26482c0dc5e0ac002daa3a01163` |
| `data.cpk::battle/bike/B11B_S.BED` | BED400 | 435888 | `1e98dd16999e763354a2b571c27cab6c494d0ffe7c54a0f87b5636b287bc1d96` |
| `data.cpk::battle/bike/B11B_W.BED` | BED400 | 456624 | `fb9a316897f390c0537c1eabd1413ff552c061eff06ba2d097ec5e43caa86b47` |
| `data.cpk::battle/boss/e200.bin::BED/e200_ENCOUNT.BED` | BED400 | 14288 | `4294f933ecf65bfba4e1b444999c903c3c47a29462602e79b53ed7a2e7ceb452` |
| `data.cpk::battle/boss/e200.bin::BED/e200_END.BED` | BED400 | 33088 | `931f2c31d66a9853504d6ad85b72593d9e62da352f97719a930c8bed0e3ab733` |
| `data.cpk::battle/boss/e200.bin::BED/e200_H160.BED` | BED400 | 9584 | `1a0d373c01cb5744cd98f6268da56e265539fe9b10a18a20d880348c77bc727a` |
| `data.cpk::battle/boss/e200.bin::BED/e200_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data.cpk::battle/boss/e200.bin::BED/e200_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e200.bin::BED/e200_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e200.bin::BED/e200_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e200.bin::BED/e200_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e201.bin::BED/e201_ENCOUNT.BED` | BED400 | 10080 | `cf6ee31d878f652f19a197d9b18502f5e32e4c765131066088aa7a008feda7bb` |
| `data.cpk::battle/boss/e201.bin::BED/e201_END.BED` | BED400 | 30192 | `b31d78e5de0092924773b1d1b2e55a1955a77c1e2175c64ea794dba20d1c32fd` |
| `data.cpk::battle/boss/e201.bin::BED/e201_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data.cpk::battle/boss/e201.bin::BED/e201_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e201.bin::BED/e201_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e201.bin::BED/e201_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e201.bin::BED/e201_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e202.bin::BED/e202_ENCOUNT.BED` | BED400 | 7344 | `5802ab6fc50884bbc4e1db4f97b92b60079582f1126c2ad60cc8e44567c4fc8c` |
| `data.cpk::battle/boss/e202.bin::BED/e202_END.BED` | BED400 | 30992 | `28078663ea649edae5bcd52b50f6e614bfb54f41f722d5d18654ea082fe50bc6` |
| `data.cpk::battle/boss/e202.bin::BED/e202_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data.cpk::battle/boss/e202.bin::BED/e202_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e202.bin::BED/e202_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e202.bin::BED/e202_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e202.bin::BED/e202_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e203.bin::BED/e203_ENCOUNT.BED` | BED400 | 8960 | `97081976c63beb477742d9e10a2a83bf02c08fbac7c5e5a0c727b332304cf803` |
| `data.cpk::battle/boss/e203.bin::BED/e203_END.BED` | BED400 | 35536 | `6221812dc3626928a9ab734835fce955f56d3164995195624b7d20907e249863` |
| `data.cpk::battle/boss/e203.bin::BED/e203_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data.cpk::battle/boss/e203.bin::BED/e203_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e203.bin::BED/e203_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e203.bin::BED/e203_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e203.bin::BED/e203_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data.cpk::battle/boss/e203.bin::BED/e203_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e204.bin::BED/e204_ENCOUNT.BED` | BED400 | 9152 | `2b707e32159c57f61dda2a51f9a4d1bb13dbaff059eb0b294294771e29b2c155` |
| `data.cpk::battle/boss/e204.bin::BED/e204_END.BED` | BED400 | 7040 | `ff3be583ba835a33b94a87a47080141ccda1aaa9cbe5946ce80a861386da0534` |
| `data.cpk::battle/boss/e204.bin::BED/e204_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data.cpk::battle/boss/e204.bin::BED/e204_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e204.bin::BED/e204_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e204.bin::BED/e204_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e204.bin::BED/e204_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data.cpk::battle/boss/e204.bin::BED/e204_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e205.bin::BED/e205_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e205.bin::BED/e205_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e205.bin::BED/e205_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e205.bin::BED/e205_ENCOUNT.BED` | BED400 | 10336 | `2f5fad7c4101340a9f82380c62dc0341cf2218734ebcfb157e4f68be0eca017b` |
| `data.cpk::battle/boss/e205.bin::BED/e205_END.BED` | BED400 | 32320 | `151da6e1d4d39174971a53d45f228d2ef3e60e9513815541c9b7dc0e03187fe1` |
| `data.cpk::battle/boss/e205.bin::BED/e205_HATUDO.BED` | BED400 | 3696 | `18feb0b9a37f5d925e6a17a49030adb5b2280a2cb28fb6261a6dd1460507cdb0` |
| `data.cpk::battle/boss/e205.bin::BED/e205_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e205.bin::BED/e205_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e205.bin::BED/e205_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e205.bin::BED/e205_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data.cpk::battle/boss/e205.bin::BED/e205_KAIJYO.BED` | BED400 | 5152 | `99bfb77d8d42d84c48c89e01526da14dd68075d99521a820c0675e0450e1d373` |
| `data.cpk::battle/boss/e205.bin::BED/e205_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e206.bin::BED/e206_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e206.bin::BED/e206_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e206.bin::BED/e206_ENCOUNT.BED` | BED400 | 6400 | `5b9ca9679564c387f222fcd3aefbba7ca6b0719893a33f2f14f35e1d09b064ce` |
| `data.cpk::battle/boss/e206.bin::BED/e206_END.BED` | BED400 | 31504 | `ee15eb497f11a707b85ad4c857527ba2732eaf4f9936fce305bbb4056de1d85d` |
| `data.cpk::battle/boss/e206.bin::BED/e206_H172.BED` | BED400 | 7552 | `74a7944243d6c20da666245d7ba3d0e76b957cb06136a5de79bbf03cd9c5f1bb` |
| `data.cpk::battle/boss/e206.bin::BED/e206_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data.cpk::battle/boss/e206.bin::BED/e206_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e206.bin::BED/e206_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e206.bin::BED/e206_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e206.bin::BED/e206_H_SP.BED` | BED400 | 4064 | `7007fbcdd175abbf881d3ae61fe12bed93872c08931f3e6b82aed5bfc83a2dc7` |
| `data.cpk::battle/boss/e206.bin::BED/e206_OFF.BED` | BED400 | 8208 | `3a6e572de45506664a54fa77e2f355094be47cc77a6b603306bad35f8b940175` |
| `data.cpk::battle/boss/e206.bin::BED/e206_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e207.bin::BED/e207_ENCOUNT.BED` | BED400 | 6944 | `e5db68919f29386aa0f2142e7da82e62bea35af0985694fb2a2ded112255088f` |
| `data.cpk::battle/boss/e207.bin::BED/e207_END.BED` | BED400 | 32096 | `cc1f9d2b5dec6f8e5b3f3fa82b5710399537f04c31bb09896f4cee7a11e8d383` |
| `data.cpk::battle/boss/e207.bin::BED/e207_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data.cpk::battle/boss/e207.bin::BED/e207_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e207.bin::BED/e207_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e207.bin::BED/e207_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e207.bin::BED/e207_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e208.bin::BED/e208_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e208.bin::BED/e208_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e208.bin::BED/e208_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e208.bin::BED/e208_ENCOUNT.BED` | BED400 | 14032 | `2275fd23ee2bea212640b51f317d363e17a98e93db834cba7a104090bb652b60` |
| `data.cpk::battle/boss/e208.bin::BED/e208_END.BED` | BED400 | 32336 | `b4a227dc5312f1dc2e4df9c6c0e5c11216ee2c24a029b1b7a209b50757dd2bde` |
| `data.cpk::battle/boss/e208.bin::BED/e208_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data.cpk::battle/boss/e208.bin::BED/e208_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e208.bin::BED/e208_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e208.bin::BED/e208_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e208.bin::BED/e208_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data.cpk::battle/boss/e208.bin::BED/e208_SK17B.BED` | BED400 | 13328 | `0d0a8231a695a1e9497d98246dfc0c544a03b95f827f3a45adb2fd3ffeb7a704` |
| `data.cpk::battle/boss/e208.bin::BED/e208_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e209.bin::BED/e209_ENCOUNT.BED` | BED400 | 9392 | `80719c0c13a8fde7ff0e545957665da6cffd72631c30e8525e0962927f919266` |
| `data.cpk::battle/boss/e209.bin::BED/e209_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data.cpk::battle/boss/e209.bin::BED/e209_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data.cpk::battle/boss/e209.bin::BED/e209_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data.cpk::battle/boss/e209.bin::BED/e209_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data.cpk::battle/boss/e209.bin::BED/e209_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data.cpk::battle/boss/e209.bin::BED/e209_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20A.bin::BED/e20A_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20A.bin::BED/e20A_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20A.bin::BED/e20A_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20A.bin::BED/e20A_ENCOUNT.BED` | BED400 | 7248 | `7ce81d540666b51712f628a3ace38d282202ae8035cd6c497bcba877582a0d26` |
| `data.cpk::battle/boss/e20A.bin::BED/e20A_END.BED` | BED400 | 30736 | `0a7fd4451e4224bbd65059a54872ed290c3a637eff66b564a19a7cafb69e82ec` |
| `data.cpk::battle/boss/e20A.bin::BED/e20A_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20A.bin::BED/e20A_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data.cpk::battle/boss/e20A.bin::BED/e20A_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20B.bin::BED/e20B_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20B.bin::BED/e20B_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20B.bin::BED/e20B_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT.BED` | BED400 | 7408 | `bfbcd65ffdff97966112ba29739ca11224d5130bd9e24793232706a5d78b80c3` |
| `data.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT2.BED` | BED400 | 6976 | `f14a2a742989b86d727615b805d73d35b92cfa2e98e4f1f2715b33c7850086aa` |
| `data.cpk::battle/boss/e20B.bin::BED/e20B_END.BED` | BED400 | 20400 | `fd46f5973ff92fc5d781a282fb094494df3e8a55809730bd91d623b8b55180d8` |
| `data.cpk::battle/boss/e20B.bin::BED/e20B_END1.BED` | BED400 | 52608 | `8069c2bcc904b6fa9ed727768508d39a56ecc95f3ce509fbb4d92d5c9c1073fb` |
| `data.cpk::battle/boss/e20B.bin::BED/e20B_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20B.bin::BED/e20B_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data.cpk::battle/boss/e20B.bin::BED/e20B_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20C.bin::BED/e20C_ENCOUNT.BED` | BED400 | 8016 | `0d7531f757e8a743070f66980adba7097a34223c7ceab5b7e15e1006db2b0f76` |
| `data.cpk::battle/boss/e20C.bin::BED/e20C_END.BED` | BED400 | 23760 | `98e314cd795aabd6134413891b58afcd5b06a5fa4c2668ab5014bb7c9e19ec9f` |
| `data.cpk::battle/boss/e20C.bin::BED/e20C_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20C.bin::BED/e20C_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e20D.bin::BED/e20D_ENCOUNT.BED` | BED400 | 6592 | `d23955927d2d12aa710e56458ce8d64c9b172cedcea6c36cd99c5696fb1d049f` |
| `data.cpk::battle/boss/e20D.bin::BED/e20D_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data.cpk::battle/boss/e20D.bin::BED/e20D_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data.cpk::battle/boss/e20D.bin::BED/e20D_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e214.bin::BED/e214_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e214.bin::BED/e214_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e214.bin::BED/e214_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e214.bin::BED/e214_ENCOUNT.BED` | BED400 | 10176 | `b56f07d78e20e9c4dbd1adc3f9c582035dd213c4a751d4a6df5889ddd7492a3d` |
| `data.cpk::battle/boss/e214.bin::BED/e214_END.BED` | BED400 | 5696 | `d109a2a14e0ee8b4fd4764e6c08aa8b78cb68e76651a42ac48c3638a0361576a` |
| `data.cpk::battle/boss/e214.bin::BED/e214_HATUDO.BED` | BED400 | 3504 | `b1792d9bd7867be710d47f27d81220d79495ec12d027191ddcd917f963e0ead6` |
| `data.cpk::battle/boss/e214.bin::BED/e214_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e214.bin::BED/e214_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e214.bin::BED/e214_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e214.bin::BED/e214_H_SP.BED` | BED400 | 3696 | `122de1e6d1504bf72f5f0fb267dc05aa3e80483c32cf8db7e8330feb9cfcce82` |
| `data.cpk::battle/boss/e214.bin::BED/e214_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e215.bin::BED/e215_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e215.bin::BED/e215_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e215.bin::BED/e215_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e215.bin::BED/e215_ENCOUNT.BED` | BED400 | 11296 | `e99453fa2913f5bbcad4d7c7b3ac29df97d5d35f5c84188af83b20bc5f4b35cf` |
| `data.cpk::battle/boss/e215.bin::BED/e215_END.BED` | BED400 | 1230864 | `c30afdd4e4cf82c8eebe4aa26faf5893a3427e5bd1595dd635f9aa63e89c9fc5` |
| `data.cpk::battle/boss/e215.bin::BED/e215_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data.cpk::battle/boss/e215.bin::BED/e215_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e215.bin::BED/e215_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e215.bin::BED/e215_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e215.bin::BED/e215_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data.cpk::battle/boss/e215.bin::BED/e215_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e216.bin::BED/e216_ENCOUNT.BED` | BED400 | 11200 | `1dcc41a7929352eb50ba07e44f68d24ed1c0a12e8cbe954e7cb75f735f1da34e` |
| `data.cpk::battle/boss/e216.bin::BED/e216_END.BED` | BED400 | 10576 | `49b1c46532ed9d3cc5a87e47736efd0ea4ec0584910f0d09d6b57ad1153d3292` |
| `data.cpk::battle/boss/e216.bin::BED/e216_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data.cpk::battle/boss/e216.bin::BED/e216_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e216.bin::BED/e216_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e216.bin::BED/e216_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e216.bin::BED/e216_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data.cpk::battle/boss/e216.bin::BED/e216_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/boss/e217.bin::BED/e217_ENCOUNT.BED` | BED400 | 6304 | `959dca87cf9853542ee07ada31ffde9a842967b3194b9a38fec7e0e912fac55c` |
| `data.cpk::battle/boss/e217.bin::BED/e217_END.BED` | BED400 | 60592 | `ea818bdc095b1797f4927266ae081698520a0b3e4b3f4c4d33693bfd57d28f82` |
| `data.cpk::battle/boss/e217.bin::BED/e217_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data.cpk::battle/boss/e217.bin::BED/e217_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data.cpk::battle/boss/e217.bin::BED/e217_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data.cpk::battle/boss/e217.bin::BED/e217_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data.cpk::battle/boss/e217.bin::BED/e217_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data.cpk::battle/btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data.cpk::battle/btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data.cpk::battle/btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data.cpk::battle/btleff.bin::ATC_D_J.BED` | BED400 | 4448 | `f9ffa0abff120b243b99b3d6006697becb10735fc24f0fb798beab2a467ece7a` |
| `data.cpk::battle/btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data.cpk::battle/btleff.bin::ATC_J_J.BED` | BED400 | 3616 | `b70ab3abd0d5491ba55fc2cb9ddfc918225ba0090e58e3aed9d396e628ce6dd3` |
| `data.cpk::battle/btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data.cpk::battle/btleff.bin::ATC_Z_J.BED` | BED400 | 6560 | `fa971120bb56cc962a0792d7af2a572779c23fa18769b814c5a531dc7e09f97f` |
| `data.cpk::battle/btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data.cpk::battle/btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data.cpk::battle/btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data.cpk::battle/btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data.cpk::battle/btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data.cpk::battle/btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data.cpk::battle/btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data.cpk::battle/btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data.cpk::battle/btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data.cpk::battle/btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data.cpk::battle/btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data.cpk::battle/btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data.cpk::battle/btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data.cpk::battle/btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data.cpk::battle/btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data.cpk::battle/btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data.cpk::battle/btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data.cpk::battle/btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data.cpk::battle/btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data.cpk::battle/btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data.cpk::battle/btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data.cpk::battle/btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data.cpk::battle/btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data.cpk::battle/btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data.cpk::battle/btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data.cpk::battle/btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data.cpk::battle/btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data.cpk::battle/btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data.cpk::battle/btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data.cpk::battle/btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data.cpk::battle/btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data.cpk::battle/btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data.cpk::battle/btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data.cpk::battle/btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data.cpk::battle/btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data.cpk::battle/btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data.cpk::battle/btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data.cpk::battle/btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data.cpk::battle/btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data.cpk::battle/btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data.cpk::battle/btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data.cpk::battle/cutin/boko/koma2/2ICON.EPL` | EPL | 19040 | `e8799a8ee97aba9524fcaf42838064d11cf6b4805c36cc1ecd960d876fdd6804` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_A_1.EPL` | EPL | 62944 | `417bd534359a5df7feaa699285b8f0fde148db3d8c6166c12c528ae1a0a79a11` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_A_1_S.EPL` | EPL | 361104 | `f4d65888661f99399158a8097ecc5b742a622ca5e1330455bad8080e1ef0213f` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_A_1_W.EPL` | EPL | 429776 | `4acbaa38f229eb56af30ad7cb2cdb52e2fd70ec4c720949a84b929f82f1a3fbe` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_A_1b.EPL` | EPL | 62512 | `407cdc57793258ac15257442814f104e62099551962252b74ad4a213a65f1cf3` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_B_2_S.EPL` | EPL | 334752 | `f0373194db48cc90a5faa114d4723ff2f7f512caaaf8ae3e32d2eb20b488bf3c` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_B_2_W.EPL` | EPL | 336800 | `8786c8151233cea74899ed76d5828629565c1398ebb665ed37416aa1e0ca325a` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_B_3_S.EPL` | EPL | 392352 | `830b1f8523844e38ccb17ae246375c312ee93dc0bb7ac566c30452e37f504cb1` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_B_3_W.EPL` | EPL | 419744 | `c5f2e318d3810cccf720449b83491721279b0126e8290775e088546aa30250e4` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_B_4_S.EPL` | EPL | 392480 | `df0e11416bc5d1c75aa64cff4c2ab43df6006d31396507e50c01b1f315e0ed2e` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_B_4_W.EPL` | EPL | 444800 | `d29a5b21cb62a439b95dc8ac8643d72a114499c8355cd2346e8161bcd61cf124` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_B_5_S.EPL` | EPL | 8704 | `527474080607a5daf66e8a3bdf7dde393dfb77d80bd558c805f45a0031b4a8bc` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_B_5_W.EPL` | EPL | 8704 | `527474080607a5daf66e8a3bdf7dde393dfb77d80bd558c805f45a0031b4a8bc` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_B_6_S.EPL` | EPL | 382944 | `8e397d707fec4d50be0e8ffe054ae49298bde2a5e211e8296d51ce5fcdcf6d44` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_B_6_W.EPL` | EPL | 381184 | `0ad5fd78481c47c3c11415bd75eb0593b8b397a23aec487ff2b3fd4eddce9307` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_B_7_S.EPL` | EPL | 365632 | `c9c03da4181568a684194daa9ad938da9013acc23b03492709e93598d9a6bf00` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_B_7_W.EPL` | EPL | 374688 | `a5b3ccca2d760fc0814c37bb53162f5f613d3fc9dce57b8cd6a826e24057b317` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_B_8_S.EPL` | EPL | 365920 | `f9a805e4eaabc8459f34d31c636ca179e15a5d5cc6a7810dc997dd4d660713fa` |
| `data.cpk::battle/cutin/boko/koma2/2KOMA_B_8_W.EPL` | EPL | 365920 | `f9a805e4eaabc8459f34d31c636ca179e15a5d5cc6a7810dc997dd4d660713fa` |
| `data.cpk::battle/cutin/boko/koma3/3ICON.EPL` | EPL | 19040 | `466b9ea93f580a2f30249dd2fc79187566548152c64813d1b05b97788452c9d8` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_A_1.EPL` | EPL | 62816 | `474228c5e6fd9116c6a8face9d7d4c8cafcf2cd5c2442db7b6d90111a560c2f1` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_A_1_S.EPL` | EPL | 401488 | `a64950ca094b274292a548cbb39b7e48c4aa35d35bfdf8e1731077fd48297481` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_A_1_W.EPL` | EPL | 456720 | `00d66352bd2e1237602f55159a8d770eb671c3f8da7c79a7ca8940d6e98d19b8` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_A_99.EPL` | EPL | 62816 | `f2869223e9fc4a849d002fe3ea7704921054ab3db1683f51706e8eadb53d241d` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_B_2_S.EPL` | EPL | 340704 | `86684a994011fcc280026090bf2faa4d4aedef8d3a8446ccdfcf718c05491864` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_B_2_W.EPL` | EPL | 325120 | `3302013ae0247e84fd56de07cfb267a9abc18d17c5c3a0586f55917640131539` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_B_3_S.EPL` | EPL | 391424 | `46332db1f3e1d0f95191efade4cc649767d9ddd4fb373445ed23ebafbb716a1c` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_B_3_W.EPL` | EPL | 387968 | `31a99b4663e6fd26f023a714ba0a0aaa9d981bc6a4379137e77f305c7b02dfa1` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_B_4_S.EPL` | EPL | 476992 | `42a49afd8779f435ac7f771681291b36b2be05ff829ef88f14ef30ea3c85b480` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_B_4_W.EPL` | EPL | 654048 | `bc987aa3dc04122893a2093b96e16e54a24f77a93a75b4ae653c617127003e54` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_B_5_S.EPL` | EPL | 8704 | `527474080607a5daf66e8a3bdf7dde393dfb77d80bd558c805f45a0031b4a8bc` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_B_5_W.EPL` | EPL | 8704 | `527474080607a5daf66e8a3bdf7dde393dfb77d80bd558c805f45a0031b4a8bc` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_B_6_S.EPL` | EPL | 382176 | `607143eb0e751c0b069b354f8cdabcf3e6d290e4b1e26d9ce533d2042216ab91` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_B_6_W.EPL` | EPL | 384672 | `09045e8401d11183f3c019a79d92e36b08a047bfa9ae4036aa977301d9a5290e` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_B_7_S.EPL` | EPL | 387008 | `1871bab1dba69fe378e26ae39996dc362ffa256cc96e2d8e91c1fbaeed0796fd` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_B_7_W.EPL` | EPL | 359008 | `74148d26dc85882a942b3175a93024a2c20c104a6812aa88fb0fb233dabbd68d` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_B_8_S.EPL` | EPL | 367168 | `037d416435b246f978c752847dc1334543d352a8430dd8fcd0720c81c5d4cf5b` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_B_8_W.EPL` | EPL | 367168 | `037d416435b246f978c752847dc1334543d352a8430dd8fcd0720c81c5d4cf5b` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_C_2_S.EPL` | EPL | 340640 | `16c174ee707dbcdcfac8b0cd14212ce3c88708b554232b12d79fa425871773e7` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_C_2_W.EPL` | EPL | 325216 | `025f3fd4bbbc1cee7ac1a5296306f496d4c0178f81ada860047abf58d795a31b` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_C_3_S.EPL` | EPL | 391680 | `773c935c2957d06f1d6d8350bd1d62487ee2dd9d697285c43488cdaba842e97a` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_C_3_W.EPL` | EPL | 387680 | `8fa3c3e24d3ee8f528231222adccd340705bb339e4b56f599a3c479c160037de` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_C_4_S.EPL` | EPL | 477376 | `5abb2b25e4d270922a39555e769ed7ab993625286e88af13fdbd6fe509ae6bd3` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_C_4_W.EPL` | EPL | 654464 | `ff5b399efc1a5545077c2aadfb41fb65cf1d47cc3f3fdc4a69226e4d99cb55c1` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_C_5_S.EPL` | EPL | 8704 | `527474080607a5daf66e8a3bdf7dde393dfb77d80bd558c805f45a0031b4a8bc` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_C_5_W.EPL` | EPL | 8704 | `527474080607a5daf66e8a3bdf7dde393dfb77d80bd558c805f45a0031b4a8bc` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_C_6_S.EPL` | EPL | 382080 | `a8003367da21843be38936703b5824d41cdd5c605208a60f258f7c60edc44306` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_C_6_W.EPL` | EPL | 384928 | `f7892002e8cc2f280c96f379d2aa83bb32314553d02af3341d4efa598c27fa2d` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_C_7_S.EPL` | EPL | 387872 | `47b1a5cf55bf90ec9878814d03c2896c7d0c9422d48bb6427b63a7a2878a7717` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_C_7_W.EPL` | EPL | 358720 | `69baf6b78ad24387e3cf6c37a87581d8e926fa9415aabce3a8c5559e3a18b3fb` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_C_8_S.EPL` | EPL | 367200 | `50b7dcb66fc2042e1c5cbe6f9aaa66ee0d385eb6b6bddbe453b320f1732bad37` |
| `data.cpk::battle/cutin/boko/koma3/3KOMA_C_8_W.EPL` | EPL | 367200 | `50b7dcb66fc2042e1c5cbe6f9aaa66ee0d385eb6b6bddbe453b320f1732bad37` |
| `data.cpk::battle/cutin/boko/koma4/4ICON.EPL` | EPL | 19040 | `9191f268dc5dc7d4430fd1696fde1b20e8ad3930945da2eef95f45c591db1366` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_A_1.EPL` | EPL | 63664 | `838b74675c7c1c1f77a6a9849e528fdbdf72ac436fe398a7521da8c988b79190` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_A_1_S.EPL` | EPL | 398800 | `68f39cb273e094ab64690de8ce8d98ea0b8192976f209e20a8de885beaff9ac6` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_A_1_W.EPL` | EPL | 454032 | `d17f894c5a4c0b4019977f36c465a48a36ae358eff67db45e404c6ebc2ec0fb8` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_A_99.EPL` | EPL | 63664 | `638a90efe589523a41d00e40dc123c76a134238ab8284cc18c5aab0a579e5593` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_B_2_S.EPL` | EPL | 334496 | `4239de422cfdb1d572f9936f727d0badf91cc037b56c92014da050247c207436` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_B_2_W.EPL` | EPL | 320448 | `d72beef34b2ac874d5ff1b7c9689ae404323fdd876a6049abe8e2de917db4813` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_B_3_S.EPL` | EPL | 372416 | `a20eea47038975b69dbeb69bcc35a73a79dfce10cf0a4cd25f1730aa406a4e77` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_B_3_W.EPL` | EPL | 388064 | `858bdd8b4b821261d45d1d18ca04c1d90adff17e86f8fbc72cc726ff5be16e02` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_B_4_S.EPL` | EPL | 477792 | `552f583ff22eaf0a7eae58f7b4921be33c0020dd2c0e7e67a6c30386a101eda1` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_B_4_W.EPL` | EPL | 657696 | `b235fc4e8038c558f494d53bdfdf82b5be7f8adb7c9bfba9ecad203a3e3a97a6` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_B_5_S.EPL` | EPL | 8704 | `527474080607a5daf66e8a3bdf7dde393dfb77d80bd558c805f45a0031b4a8bc` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_B_5_W.EPL` | EPL | 8704 | `527474080607a5daf66e8a3bdf7dde393dfb77d80bd558c805f45a0031b4a8bc` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_B_6_S.EPL` | EPL | 382240 | `2dc2b6e65ab48a21a9a37685bcb0e12be43341e835df123c9fddae3df268f7bb` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_B_6_W.EPL` | EPL | 384192 | `6c67ecc04bd38e8420b103721f696b04d83b3d18d568626c7fd4b7345ac34e8c` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_B_7_S.EPL` | EPL | 388064 | `f0d6213052117e9eb30c10c6e5503106248fcbce3bad5b390d4b83c2dcd9f7db` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_B_7_W.EPL` | EPL | 358912 | `9f3d9b67c33f5748ff215e1c2080697944609f5aa57fb764ea6b547e685666af` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_B_8_S.EPL` | EPL | 367488 | `844436f789fe4d96a7b2db791aed521789e9228f38b2186a03e8c3dc5e3cd804` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_B_8_W.EPL` | EPL | 367488 | `844436f789fe4d96a7b2db791aed521789e9228f38b2186a03e8c3dc5e3cd804` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_C_2_S.EPL` | EPL | 334368 | `0f89b2b5ffa93979b91dbf3206059bef21034666a0e90d323a9bd17c74a27e93` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_C_2_W.EPL` | EPL | 321376 | `cd265af2c6989e5c3f141059da369a5d64a56cb6bdd59b1e228ffa7623244683` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_C_3_S.EPL` | EPL | 372768 | `e3ce2311d2022a9bb5c1a1613447dd3a733b17c10fcc2cd8c7ba19cc39f33a97` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_C_3_W.EPL` | EPL | 387936 | `db927514f93d03e5d813edd5a9ad069c23c914b8286951a190edbdc545d2c638` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_C_4_S.EPL` | EPL | 479008 | `c8c7570da07fd69fd7cb5a022b9d2ba59980cc342d7ce0660836797d621cc66d` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_C_4_W.EPL` | EPL | 659008 | `76291c941730748f395355573d5c010af52b2608ceebe79063c8ae21da88e3fd` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_C_5_S.EPL` | EPL | 8704 | `527474080607a5daf66e8a3bdf7dde393dfb77d80bd558c805f45a0031b4a8bc` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_C_5_W.EPL` | EPL | 8704 | `527474080607a5daf66e8a3bdf7dde393dfb77d80bd558c805f45a0031b4a8bc` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_C_6_S.EPL` | EPL | 382752 | `e753ca7388d02d491434c9da5f07037c9bf8d9bbd30f0761a5e4ab498deb2d41` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_C_6_W.EPL` | EPL | 385152 | `8368f403d73a51444928316b55b1d94cedd86ae7d37f9fa2c03144a5962e9138` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_C_7_S.EPL` | EPL | 388832 | `cebea138d99a83a54cf13a67e75b021a49ab95c1ba6da356057eb9c431780be7` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_C_7_W.EPL` | EPL | 359712 | `b673d050473639673e66de6ef4c6c960bfe7641f60877f4c2d6767aaa7aee148` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_C_8_S.EPL` | EPL | 368384 | `b7f6597b4411aac4b5872ef3735ee76959761bf6ac0f3d9f6552523b5d486b7d` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_C_8_W.EPL` | EPL | 368384 | `b7f6597b4411aac4b5872ef3735ee76959761bf6ac0f3d9f6552523b5d486b7d` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_D_2_S.EPL` | EPL | 334784 | `c86720db8ad0d165bffcf1a066c1fcb78e7e6e01f78dcb9838237c6291e68d22` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_D_2_W.EPL` | EPL | 319328 | `baeeb879713f2705834133d52fd5f8d70dd9b8790c1dc4855f0c9d1c1f01239a` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_D_3_S.EPL` | EPL | 390848 | `be16eae53f25ba15c52c4e512658017e479a11ee938b7b991e87a803ecb2465a` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_D_3_W.EPL` | EPL | 385888 | `f0876d3e1dfae7f2283d8c9c15a13aa47e3227881bb5a74d0930f205d9d78f0b` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_D_4_S.EPL` | EPL | 477984 | `93bf1ef0d872e430838bebaf83797e82c638159342030cf952031df2ff75fb88` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_D_4_W.EPL` | EPL | 654784 | `df2c0e84ecce11cb6d721fea4821e6fba31a7c32596a8c108a5efac77252207b` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_D_5_S.EPL` | EPL | 8704 | `527474080607a5daf66e8a3bdf7dde393dfb77d80bd558c805f45a0031b4a8bc` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_D_5_W.EPL` | EPL | 8704 | `527474080607a5daf66e8a3bdf7dde393dfb77d80bd558c805f45a0031b4a8bc` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_D_6_S.EPL` | EPL | 381440 | `380b119ba5c6e8aac3bf57ee41de6e8890c9ce4a1f9614d7a2d7104d1921c9e7` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_D_6_W.EPL` | EPL | 380704 | `64039acf2211fc580c12357d596036ed220015e2209a7b537aa467e63f57af56` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_D_7_S.EPL` | EPL | 383040 | `7050e946355eadc01dfa890a964d2518a25cc9dc5d877bd31e419275f0dcfcca` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_D_7_W.EPL` | EPL | 355616 | `9719638f866b5ce5c090683c2e8a502336635ced118d1138bfbd0f7045f46057` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_D_8_S.EPL` | EPL | 348576 | `7f2089113e0c3961f48d435fbe0ece675ae28610db915597ffc3736cb4d097a0` |
| `data.cpk::battle/cutin/boko/koma4/4KOMA_D_8_W.EPL` | EPL | 348576 | `7f2089113e0c3961f48d435fbe0ece675ae28610db915597ffc3736cb4d097a0` |
| `data.cpk::battle/cutin/syoudaku/BOKO_SD_2_S.EPL` | EPL | 144016 | `3f6f50a94baa02420895fb84d1247ce4a189528c8a1814cae79ce4ae0263ba07` |
| `data.cpk::battle/cutin/syoudaku/BOKO_SD_2_W.EPL` | EPL | 144016 | `6c0e7451b28b5287013c718f99826e7d55e6ea694428e56d7500b518004bd113` |
| `data.cpk::battle/cutin/syoudaku/BOKO_SD_3_S.EPL` | EPL | 144016 | `06b904ca0345403151660b105975c77e4e6a40aae32193a23cb6b92273f14a09` |
| `data.cpk::battle/cutin/syoudaku/BOKO_SD_3_W.EPL` | EPL | 144016 | `068c5edc3dc051bd7ec76ce7d43c2e3153908430091ddd1a45b5a28f6052b4e9` |
| `data.cpk::battle/cutin/syoudaku/BOKO_SD_4_S.EPL` | EPL | 144016 | `ee26922e11181190a0e9f700c6a91d295c190a7039ed7c2751d818f155bd41d9` |
| `data.cpk::battle/cutin/syoudaku/BOKO_SD_4_W.EPL` | EPL | 144016 | `a3adf90e5f58e6a704501d17b408ac0ca2969ed7b70d75cd11b8e622633e43c9` |
| `data.cpk::battle/cutin/syoudaku/BOKO_SD_5_S.EPL` | EPL | 9408 | `745223f0d583995080f562bb680645ff4f8f04ad69840f8bd3426fbb7b5bc9ca` |
| `data.cpk::battle/cutin/syoudaku/BOKO_SD_5_W.EPL` | EPL | 9408 | `745223f0d583995080f562bb680645ff4f8f04ad69840f8bd3426fbb7b5bc9ca` |
| `data.cpk::battle/cutin/syoudaku/BOKO_SD_6_S.EPL` | EPL | 144016 | `776c50e6c7cbe2747591eb73d474ca637ff1812b98e454b074faf849ac71824d` |
| `data.cpk::battle/cutin/syoudaku/BOKO_SD_6_W.EPL` | EPL | 144016 | `6665dd8a01396001374b3703f259c960d8bacf23c76220b44bad5c4a33ea4d58` |
| `data.cpk::battle/cutin/syoudaku/BOKO_SD_7_S.EPL` | EPL | 144016 | `f220df68f00bd0a178601b5925f9e1cab774fd64abcc47c0c5e296a4812c918d` |
| `data.cpk::battle/cutin/syoudaku/BOKO_SD_7_W.EPL` | EPL | 144016 | `22990b0ffa41b0775112c53fbe77003e8aef7edc053210677433332ebee87d27` |
| `data.cpk::battle/cutin/syoudaku/BOKO_SD_8_S.EPL` | EPL | 143984 | `1d0b3133295fa74ef536e616045b5a0dff9a3a22797f646ca72ea2fbffc2cef7` |
| `data.cpk::battle/cutin/syoudaku/BOKO_SD_8_W.EPL` | EPL | 143984 | `1d0b3133295fa74ef536e616045b5a0dff9a3a22797f646ca72ea2fbffc2cef7` |
| `data.cpk::battle/cutin/syoudaku/TUGK_SD_2_S.EPL` | EPL | 144016 | `1924892b10090a26ea23420a09f6ca4dfd6a23c7e33a023a42941fe237a57755` |
| `data.cpk::battle/cutin/syoudaku/TUGK_SD_2_W.EPL` | EPL | 144016 | `e253f7ccef791ca8b42a41537d522848e510590c7ab0698618e9ce8bcb08f4c3` |
| `data.cpk::battle/cutin/syoudaku/TUGK_SD_3_S.EPL` | EPL | 144016 | `230cf8df3738bacfb0549daae56ec6b88a6448378e81dadefc47225ed0827120` |
| `data.cpk::battle/cutin/syoudaku/TUGK_SD_3_W.EPL` | EPL | 144016 | `9d16e18268d47b04d0b8e283f2f9df305c1354f45153496f31162f1996701eca` |
| `data.cpk::battle/cutin/syoudaku/TUGK_SD_4_S.EPL` | EPL | 144016 | `df282b03db114f142b341ec19e3bcaffc41dfacaea55b99de2eb8a8ef9acfb94` |
| `data.cpk::battle/cutin/syoudaku/TUGK_SD_4_W.EPL` | EPL | 144016 | `ef969f6b4610119606168021379fcabfedb065a93ebea6aa30ec7bfeff54f6a9` |
| `data.cpk::battle/cutin/syoudaku/TUGK_SD_5_S.EPL` | EPL | 9408 | `745223f0d583995080f562bb680645ff4f8f04ad69840f8bd3426fbb7b5bc9ca` |
| `data.cpk::battle/cutin/syoudaku/TUGK_SD_5_W.EPL` | EPL | 9408 | `745223f0d583995080f562bb680645ff4f8f04ad69840f8bd3426fbb7b5bc9ca` |
| `data.cpk::battle/cutin/syoudaku/TUGK_SD_6_S.EPL` | EPL | 144016 | `9ecdf77e714678de1efb3b30444058301c5704409cc96de4b5b5792f82a3f938` |
| `data.cpk::battle/cutin/syoudaku/TUGK_SD_6_W.EPL` | EPL | 144016 | `58264c921713aa85e79e36f3713fd51475f60d3b48841ee502bb849ee88c45ca` |
| `data.cpk::battle/cutin/syoudaku/TUGK_SD_7_S.EPL` | EPL | 144016 | `71647af0cac73221d2ca49d626111720c98681170b9b67b7fb89367509e5f044` |
| `data.cpk::battle/cutin/syoudaku/TUGK_SD_7_W.EPL` | EPL | 144016 | `4b7b7e00d3dcc48f53f81952fd8a56756a2182b4457311c6530237fd39506bfa` |
| `data.cpk::battle/cutin/syoudaku/TUGK_SD_8_S.EPL` | EPL | 143984 | `4846985dd173e2e2252290abfa4f981cbbe3a8d13dcae80d8a4e15e6a001b499` |
| `data.cpk::battle/cutin/syoudaku/TUGK_SD_8_W.EPL` | EPL | 143984 | `4846985dd173e2e2252290abfa4f981cbbe3a8d13dcae80d8a4e15e6a001b499` |
| `data.cpk::battle/cutin/syoukan/SYKN_1.EPL` | EPL | 144336 | `72f36c0fe840b6e2a7fb364c6a698f2615bdd7d030f0c43dbe0612b6c3ceb7f6` |
| `data.cpk::battle/cutin/syoukan/SYKN_108.EPL` | EPL | 42192 | `679a3ee0c8dbfa231d0042cbf5f39cd47b12e674c0c1de701ace4f1a5a323eab` |
| `data.cpk::battle/cutin/syoukan/SYKN_10B.EPL` | EPL | 42192 | `ddaee7359ac80096ef823c036dda649686717d06621ede5433763cc943faf035` |
| `data.cpk::battle/cutin/syoukan/SYKN_10C.EPL` | EPL | 42192 | `477e27126270249a1c11d5dc95fec1a687c0e419f6e7e91a961f8409eb20357c` |
| `data.cpk::battle/cutin/syoukan/SYKN_126.EPL` | EPL | 42192 | `36520329451e1035904585b78e6a223e9542e004676f261cc121f1bb1439b102` |
| `data.cpk::battle/cutin/syoukan/SYKN_12A.EPL` | EPL | 42192 | `170d6f45a216c559346cd761fd1d404672e42222fba2f932b32ee2fe7dd5a970` |
| `data.cpk::battle/cutin/syoukan/SYKN_12B.EPL` | EPL | 42192 | `719226171ad00f4f4ceeffab498fb47af065f16dacc9a8f632300833756c2716` |
| `data.cpk::battle/cutin/syoukan/SYKN_2.EPL` | EPL | 144432 | `21d5085974cd08ddf1fa3dd11a21160a14d430a642cfd5c4cb19d8bab42e3dae` |
| `data.cpk::battle/cutin/syoukan/SYKN_3.EPL` | EPL | 144624 | `012d5e38c2ae4867d6c8a36d695a0f6f2fa9657deee824e17d5a8c0104526c65` |
| `data.cpk::battle/cutin/syoukan/SYKN_4.EPL` | EPL | 144848 | `ea9c5d6d0038124935a3bf59fc7398bcda31d8cec4bdb695bc877d19ac2720c7` |
| `data.cpk::battle/cutin/syoukan/SYKN_5.EPL` | EPL | 278944 | `346d92a1980022e90aa4bc8e12c3e1623676b3400f474b0f015203a680e0eb92` |
| `data.cpk::battle/cutin/syoukan/SYKN_6.EPL` | EPL | 144400 | `f448dab6ef04b58125381db819285cc6971395403af83adbd8d9ec6b16e77617` |
| `data.cpk::battle/cutin/syoukan/SYKN_63.EPL` | EPL | 42176 | `0e2d677d081920f11c1978095c573bd010b17c17590f201eaf389282fbe368ea` |
| `data.cpk::battle/cutin/syoukan/SYKN_7.EPL` | EPL | 143472 | `caeb610af1fe88bd763a05ec70f9420f3b99a598a1cd9ccbd4a9a8c100a6c28d` |
| `data.cpk::battle/cutin/syoukan/SYKN_8.EPL` | EPL | 140368 | `241758ea8f150238e9ff773dd25a214b87b551abd00eab0f25727ad9ef3d71dc` |
| `data.cpk::battle/cutin/syoukan/SYKN_9.EPL` | EPL | 42176 | `768d4a178d9049627c0b5d18b2692386589c3ceec2605db05ffb428415f70213` |
| `data.cpk::battle/cutin/syoukan/SYKN_A.EPL` | EPL | 42176 | `aab0159ecd81362683cf3014b099667155c2247d22a549af4ad31e1b05bdfeec` |
| `data.cpk::battle/cutin/syoukan/SYKN_E109.EPL` | EPL | 173104 | `2c274b53ec39a9595130bdd371fad189307b902ab1e7fa8e153f9e458ddee28d` |
| `data.cpk::battle/cutin/tokusyu/TOKUS_2_S.EPL` | EPL | 296432 | `8e45c41416a25ee3caf8e05bc8e130030420d57d83504dc186da810268c18df8` |
| `data.cpk::battle/cutin/tokusyu/TOKUS_2_W.EPL` | EPL | 296416 | `fea3a09ecd17a6f4640e828f73936ed7741fe810852fad63f45e9fdd88b28969` |
| `data.cpk::battle/cutin/tokusyu/TOKUS_3_S.EPL` | EPL | 296432 | `2398e1504466b1c14d70f1213440a51284d1d691afc27439966ebb01e8515fd5` |
| `data.cpk::battle/cutin/tokusyu/TOKUS_3_W.EPL` | EPL | 296416 | `d67d6bbfc1dd9b7fdda315673a3e7ff66c21e6c95d6c915b42d6a5a3f7eaad37` |
| `data.cpk::battle/cutin/tokusyu/TOKUS_4_S.EPL` | EPL | 296432 | `6b57cd81b9cc7886d5ff5e1c2520d4a8fa92492a68f5f6ec0c7b6f2d6a98d199` |
| `data.cpk::battle/cutin/tokusyu/TOKUS_4_W.EPL` | EPL | 296432 | `58e29514c023f52c37d91458823faf325430774d8b2877742cbfef4b0a32fd10` |
| `data.cpk::battle/cutin/tokusyu/TOKUS_5_S.EPL` | EPL | 831584 | `9ac7832fbdb00e2b30b083f7252ea0c9df0558e558b494b2d7b1ae3583685147` |
| `data.cpk::battle/cutin/tokusyu/TOKUS_5_W.EPL` | EPL | 831552 | `8dda3baa0d14800d29605472d4b70ddbf7e4e71aafcbca7b84c85a53ea7208ac` |
| `data.cpk::battle/cutin/tokusyu/TOKUS_6_S.EPL` | EPL | 296416 | `6f8eb56afcffd4d554362976217b343e7184073e74fea00966ea40cbc93e614c` |
| `data.cpk::battle/cutin/tokusyu/TOKUS_6_W.EPL` | EPL | 296848 | `e5239c8794161a019a86ecbcff4535458963c3e2d512370676eb022c13a44719` |
| `data.cpk::battle/cutin/tokusyu/TOKUS_7_S.EPL` | EPL | 296416 | `c78a31612ea176bade22d7f23185c0b5c59f4fb6f2ff92347c38643321324fb8` |
| `data.cpk::battle/cutin/tokusyu/TOKUS_7_W.EPL` | EPL | 296416 | `2fbc22e692d9e03fdcf69e7800eff5ff5e877ddbdbf185a8991536a7487b6500` |
| `data.cpk::battle/cutin/tokusyu/TOKUS_8_S.EPL` | EPL | 296416 | `463401592021af490188bc180e776b441683ef4ba67024cd67ebf9a063fc21f8` |
| `data.cpk::battle/cutin/tokusyu/TOKUS_8_W.EPL` | EPL | 296416 | `463401592021af490188bc180e776b441683ef4ba67024cd67ebf9a063fc21f8` |
| `data.cpk::battle/effect/AGI_CRT.BED` | BED400 | 3120 | `093003385e871e161f06b3566ea1c54efc5eac1ff9520cda1a7400357d2e6c0f` |
| `data.cpk::battle/effect/ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data.cpk::battle/effect/ATC_D.BED` | BED400 | 9840 | `e72c0edf0f36792e841793a47720f485c31155529cf3dda2a3761c6c64254063` |
| `data.cpk::battle/effect/ATC_D_J.BED` | BED400 | 4448 | `f9ffa0abff120b243b99b3d6006697becb10735fc24f0fb798beab2a467ece7a` |
| `data.cpk::battle/effect/ATC_J.BED` | BED400 | 10768 | `5006da5253f5245c5e096e34f79aceece761d58ae9baf96ad4be8b27c9209538` |
| `data.cpk::battle/effect/ATC_J_J.BED` | BED400 | 3616 | `b70ab3abd0d5491ba55fc2cb9ddfc918225ba0090e58e3aed9d396e628ce6dd3` |
| `data.cpk::battle/effect/ATC_Z.BED` | BED400 | 9328 | `1abd64a1365aacc09397280f5bc3908015e5cda4000908f2e2c8cc38ecc306ad` |
| `data.cpk::battle/effect/ATC_Z_J.BED` | BED400 | 6544 | `8c0eac1041764b959eb1ecc3f8f74de4650f28fbf7b04f4a13325a2f5fba667f` |
| `data.cpk::battle/effect/AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data.cpk::battle/effect/AUTO_DK.BED` | BED400 | 2896 | `e42022eb3728208efb3b0e0954d1968fd000c3edb5f1dbc4bf1d524dd342d6ba` |
| `data.cpk::battle/effect/AUTO_HJ.BED` | BED400 | 3216 | `715aa585f9082d8899a530402adf0690d55e4f5715662b5dd70bd6c0bc04548d` |
| `data.cpk::battle/effect/AUTO_SA.BED` | BED400 | 3440 | `68fad7e1cb7c9544ec188ae9995bd3e200415487ad9b86a81558f5685549ffc7` |
| `data.cpk::battle/effect/AUTO_SR.BED` | BED400 | 3344 | `05f4eeaf502f3eec8de3c93406229143ce418af626953eeb0526af8b0c316560` |
| `data.cpk::battle/effect/AYATURU.BED` | BED400 | 5200 | `36445b4c9545a9020655283837fed818d4ead826a4e71b3d2f81e6e7c99142a0` |
| `data.cpk::battle/effect/BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data.cpk::battle/effect/BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data.cpk::battle/effect/CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data.cpk::battle/effect/COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data.cpk::battle/effect/CRT_HP.BED` | BED400 | 3904 | `a1f59099af53e9b6ffad4a44d108199adbffb9cc152a45dff203c00bf14d7d2d` |
| `data.cpk::battle/effect/DEATH.BED` | BED400 | 3184 | `4284e5f83bf938ad355cf44a22a78301e9b97802743ba80a47c2c8e3c889d3cd` |
| `data.cpk::battle/effect/DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data.cpk::battle/effect/DOWN_J.BED` | BED400 | 5648 | `af6dcc5749abab25ff37fde88099f501b650a2ab028e8cb0f5be698ad9caded2` |
| `data.cpk::battle/effect/DOWN_S.BED` | BED400 | 4880 | `699e3a83242e2ee263d38d5a1e064bbb28772b736d082a3f4dff62cbdc141c60` |
| `data.cpk::battle/effect/DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data.cpk::battle/effect/FUTOBI_S.BED` | BED400 | 1584 | `14c19b0f7efec23a59696a0f00025873e66300ce4dc9df2b3ad6c47bd678e600` |
| `data.cpk::battle/effect/GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data.cpk::battle/effect/GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data.cpk::battle/effect/HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data.cpk::battle/effect/HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data.cpk::battle/effect/HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data.cpk::battle/effect/HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data.cpk::battle/effect/HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data.cpk::battle/effect/ITEM.BED` | BED400 | 3504 | `012b813e42fc8072bc8146d229e3caaaf350f600fd7774361a00869e381fcd78` |
| `data.cpk::battle/effect/KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data.cpk::battle/effect/KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data.cpk::battle/effect/KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data.cpk::battle/effect/PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data.cpk::battle/effect/PER.BED` | BED400 | 2224 | `221479dbd158572a564003ee878188499f135ff0f435a912c7fee81b01a28728` |
| `data.cpk::battle/effect/POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data.cpk::battle/effect/ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data.cpk::battle/effect/R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data.cpk::battle/effect/SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data.cpk::battle/effect/SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data.cpk::battle/effect/SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data.cpk::battle/effect/SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data.cpk::battle/effect/SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data.cpk::battle/effect/SYUCYU.BED` | BED400 | 5136 | `194f9af06049c496e0fd3e37bf3fee41dea4637420fc95b3ac0ada71e3d205b2` |
| `data.cpk::battle/effect/TUIGEKI.BED` | BED400 | 2512 | `9a1b106caa05874c850e97fe452109789200a5be4f21aa5788caac94e240b558` |
| `data.cpk::battle/effect/TUKEKAE.BED` | BED400 | 4048 | `0bb7da762668a9188eba6b7f870b43f8578cb9178413fd758008941821fc320e` |
| `data.cpk::battle/effect/ZIO_CRT.BED` | BED400 | 7792 | `882f3e4929c57a6d7edc5520f6065d78bdfee9b9dabcd68bccb158919d92f9bc` |
| `data.cpk::battle/result/shuffle.arc::SHUFFLE01.EPL` | EPL | 6368 | `bb7e2d674c4b18778f6f7b02304159ee7a192b90f38406353bd32754207244b5` |
| `data.cpk::battle/result/shuffle.arc::SHUFF_01.EPL` | EPL | 8864 | `59627e77df24e98b1aec5cf22a9c47495e145505c6fea06b5b0429b3427a7fad` |
| `data.cpk::battle/result/shuffle.arc::SHUFF_02.EPL` | EPL | 9760 | `58c0739b468638264c6d3534bd2b2c6f5af07a3e6d878fc4104351a4f01b9dad` |
| `data.cpk::battle/result/shuffle.arc::SHUFF_03.EPL` | EPL | 14688 | `c50a3a6c14cb67013b24aa7f12833c401ef5ef7f5bb823ec4a45916b1d996782` |
| `data.cpk::battle/result/shuffle.arc::SHUFF_04.EPL` | EPL | 9568 | `2ac489ffa035960b485696279cdcdc702e512321dfec3c5de3e0533354ac27ef` |
| `data.cpk::battle/result/shuffle.arc::SHUFF_05.EPL` | EPL | 10144 | `1c02f0750e4a6e99932d04af21a9752ae50b80597201fbc7ce944bdde2b45432` |
| `data.cpk::battle/result/shuffle.arc::SHUFF_06.EPL` | EPL | 15104 | `a7756c94b0471c8bedc1447c499df3233be21f8992655205858fb1e97e7d7c96` |
| `data.cpk::battle/result/shuffle.arc::SHUFF_09.EPL` | EPL | 1600 | `7db6177b81d30997b063517caf3b0a1a1781a7363f3219940aaa1d3deef01bb4` |
| `data.cpk::battle/union/U15D_A.BED` | BED400 | 229136 | `635839279fd1586c9ea97162d2b5515d9bc8fe33378b9a04ecf9a14e02e06370` |
| `data.cpk::battle/union/U15D_B.BED` | BED400 | 20384 | `17912421ffabe8a17a19cb1c458ada80656db615d3d9c947f27fc67204aee57a` |
| `data.cpk::battle/union/U15E_A.BED` | BED400 | 262672 | `24973476ce9f7603925f5c752fa8a0752554dc5275277949345f2802ab1ef241` |
| `data.cpk::battle/union/U15E_B.BED` | BED400 | 20112 | `b18f1b83aca60f1cab1c98244e55bd5e2137f06dc6850c8da97de30bb0599ccd` |
| `data.cpk::battle/union/U15F_A.BED` | BED400 | 251840 | `e5b46c26e0e4369cd7f6807db10973df5ec29610737a7997b686aebd32861b6e` |
| `data.cpk::battle/union/U15F_B.BED` | BED400 | 21968 | `8af7a440fb94bc46f023fc1f91a170269812353e36466eec859c7f056195323a` |
| `data.cpk::cutin/C0_0.EPL` | EPL | 8704 | `7f0728aae2954c216473c47ed649e35a54bef30d263a628ffdc3e72a605ab8a4` |
| `data.cpk::cutin/C10_1.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C10_2.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C11_1.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C11_2.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C11_3.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C11_4.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C12_1.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C12_2.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C12_3.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C12_4.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C14_1.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C14_2.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C14_3.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C14_4.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C2_1.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C2_10.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C2_2.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C2_20.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C2_21.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C2_22.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C2_23.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C2_3.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C2_30.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C2_31.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C2_32.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C2_33.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C2_4.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C2_5.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C2_6.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C2_7.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C2_8.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C2_9.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C3_1.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C3_2.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C3_20.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C3_21.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C3_22.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C3_23.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C3_3.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C3_30.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C3_31.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C3_32.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C3_33.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C3_4.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C3_5.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C3_6.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C4_1.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C4_2.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C4_20.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C4_21.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C4_22.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C4_23.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C4_3.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C4_30.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C4_31.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C4_32.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C4_33.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C4_4.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C4_5.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C4_6.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C5_1.EPL` | EPL | 61328 | `8a59b0f28ea92c9eb8f719b9af31b311bef591963e7afa6a9b5aadf1643b515a` |
| `data.cpk::cutin/C5_2.EPL` | EPL | 61328 | `8a59b0f28ea92c9eb8f719b9af31b311bef591963e7afa6a9b5aadf1643b515a` |
| `data.cpk::cutin/C5_20.EPL` | EPL | 111088 | `708d5a98627f61e889edb5cdf5d2d0357a2583cace45f994eb4e05cd0724d3f0` |
| `data.cpk::cutin/C5_21.EPL` | EPL | 111088 | `fc5f1997e308a9cb6f865880c7fd697a32b1a593562f2c302b4f71b6ccd80185` |
| `data.cpk::cutin/C5_22.EPL` | EPL | 111088 | `5f559c85d93533b372022b9ef306a5df4fa0894b0d9eb673af3ca59a7f58fdf0` |
| `data.cpk::cutin/C5_23.EPL` | EPL | 111088 | `58de4c254960333ec2355daa631ee9ca06b809a72665e29093c431b9e027a1ed` |
| `data.cpk::cutin/C5_3.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C5_30.EPL` | EPL | 111088 | `6d9b837ac7a21a7d9c0ef227e0656996630a7d36bca00eaa81687fcb68895a49` |
| `data.cpk::cutin/C5_31.EPL` | EPL | 111088 | `10547b86fec55234bfa637113190e9ac8dc43387897be51c215f23e2ee62f51a` |
| `data.cpk::cutin/C5_32.EPL` | EPL | 111088 | `e5f0d1695b5a044b3fd4381d272074b1d930c2e002ad70f3441e07571c25d885` |
| `data.cpk::cutin/C5_33.EPL` | EPL | 111088 | `25152668067e6b2e94084222753c92b122c136b7423c163858ec5b720117caab` |
| `data.cpk::cutin/C5_4.EPL` | EPL | 94800 | `921ec1cf133858770630ee88131b110e6c916735554f867d37eaba2454960ea2` |
| `data.cpk::cutin/C5_40.EPL` | EPL | 111136 | `aa6c5af35e193df286bf5faf37a6fd3718926540e2283458399855c1b0d40d7f` |
| `data.cpk::cutin/C5_41.EPL` | EPL | 111136 | `ddb77d0e711647dcb5653af79ff4462926f6fe6baa32f61f72bc940d8b9569bc` |
| `data.cpk::cutin/C5_5.EPL` | EPL | 94800 | `921ec1cf133858770630ee88131b110e6c916735554f867d37eaba2454960ea2` |
| `data.cpk::cutin/C5_6.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C6_1.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C6_2.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C6_20.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C6_21.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C6_22.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C6_23.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C6_3.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C6_30.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C6_31.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C6_32.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C6_33.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C6_4.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C6_5.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C6_6.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C7_1.EPL` | EPL | 61328 | `552a914540150c706ce69600336f20d26f2a3568fe2d88efba4e57b6669710e4` |
| `data.cpk::cutin/C7_2.EPL` | EPL | 61328 | `552a914540150c706ce69600336f20d26f2a3568fe2d88efba4e57b6669710e4` |
| `data.cpk::cutin/C7_20.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C7_21.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C7_22.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C7_23.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C7_3.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C7_30.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C7_31.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C7_32.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C7_33.EPL` | EPL | 206768 | `da5c6d1229250bb9f474d21ed5afce5ea052bce1c8b12686080c1a8b0e5e68a7` |
| `data.cpk::cutin/C7_4.EPL` | EPL | 94800 | `6da081c4b8b506dc4abbe80641d2e457de712590b2dd013ae5aba34386e73aa0` |
| `data.cpk::cutin/C7_5.EPL` | EPL | 94800 | `6da081c4b8b506dc4abbe80641d2e457de712590b2dd013ae5aba34386e73aa0` |
| `data.cpk::cutin/C7_6.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C8_1.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C8_2.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C8_20.EPL` | EPL | 110816 | `be3b967d6e6aad6d8fe9c4a93f084d241628807d605dfb2369a635bcac26a248` |
| `data.cpk::cutin/C8_21.EPL` | EPL | 110832 | `2832506f641a0cbf9847e2fa36b42fb30ec40e3cd24907088396021d05f7f71f` |
| `data.cpk::cutin/C8_22.EPL` | EPL | 110816 | `bbdb5de1ac353448d41ac0fd1a189307bc9eef21bad052d158776e6ba96460c2` |
| `data.cpk::cutin/C8_23.EPL` | EPL | 110816 | `83e14f2133bc42749871602c2b69a0fae7bc0fb3af4189ad1bc5c671a83492e4` |
| `data.cpk::cutin/C8_3.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C8_30.EPL` | EPL | 110816 | `be3b967d6e6aad6d8fe9c4a93f084d241628807d605dfb2369a635bcac26a248` |
| `data.cpk::cutin/C8_31.EPL` | EPL | 110832 | `2832506f641a0cbf9847e2fa36b42fb30ec40e3cd24907088396021d05f7f71f` |
| `data.cpk::cutin/C8_32.EPL` | EPL | 110816 | `bbdb5de1ac353448d41ac0fd1a189307bc9eef21bad052d158776e6ba96460c2` |
| `data.cpk::cutin/C8_33.EPL` | EPL | 110816 | `83e14f2133bc42749871602c2b69a0fae7bc0fb3af4189ad1bc5c671a83492e4` |
| `data.cpk::cutin/C8_4.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C8_5.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::cutin/C8_6.EPL` | EPL | 91744 | `8817afce6fb7b3792cc23c968f6973c4b0f43cff79c41043ef7eb5ca4980ed60` |
| `data.cpk::event/fishing.bin::FLOAT_01.EPL` | EPL | 7232 | `3f7721ea180386dc79379572ea567e7521b2a470d9f76167aa71bbdebdb3f5d0` |
| `data.cpk::event/fishing.bin::FLOAT_WV.EPL` | EPL | 7264 | `db0358e93caaab0ccfe666c308ece5ff9acc4522ae95044fbc00be249dadb346` |
| `data.cpk::event/fishing.bin::SPRAY_01.EPL` | EPL | 11424 | `6229b4e3eae59c7f62ee785737578b3aba127205f979dd528d31222cb60d0be3` |
| `data.cpk::event/fishing.bin::SPRAY_02.EPL` | EPL | 4288 | `54cabd45d19febb6d5decaeb2ee9c081f8c34a1e3273c2db20449cc0fb637aa1` |
| `data.cpk::event/fishing.bin::SPRAY_03.EPL` | EPL | 23296 | `34836ad2132b20984fab062cf62efbae171cd5438d7d64d11d3dbe06661d15f3` |
| `data.cpk::event/fishing.bin::SPRAY_04.EPL` | EPL | 22880 | `9d383ff669c401e9ff458d0904761040d95cfbd90b64b6aef2bf04dded321238` |
| `data.cpk::event/fishing.bin::WAVE_01.EPL` | EPL | 512 | `e73f641168a4d70f2cfcb10cf9d4bbb71fcf1466208de78c682d22e8ba876bb3` |
| `data.cpk::event/fishing.bin::WAVE_02.EPL` | EPL | 512 | `e623b56cce128bbc917f276a6238991b9417971c74de892194acc4e17610a982` |
| `data.cpk::event/fishing.bin::WAVE_03.EPL` | EPL | 864 | `7431335642b1c77dc423964534743a10b1c9193279f2e36078cba7315e7a8fd4` |
| `data.cpk::event/fishing.bin::WAVE_04.EPL` | EPL | 82368 | `5975b6978e545c03916310565458748458e384874ecd227887f43dc0eb331c5f` |
| `data.cpk::event/fishing.bin::WAVE_05.EPL` | EPL | 14752 | `362d3e3b9d86e96cc408b2ac392838551848ecb27d829fe2996d1581b7132cfc` |
| `data.cpk::event/fishing.bin::WAVE_06.EPL` | EPL | 15232 | `7f2fc8f2c4b6baf6b48cecabdfb1ab1c09b3971cab53ea9933f4df36846d71f9` |
| `data.cpk::event_data/community.bin::CARD_30.EPL` | EPL | 267120 | `51a878d3a3d380b26abeab633ad5e075894c519e3fa3436cce36af0d81cc6c91` |
| `data.cpk::event_data/community.bin::CARD_EF04.EPL` | EPL | 153600 | `feb474459c1fb4b4d3b11ea541d82ea2673a32a14118275c43e0430a21ec9311` |
| `data.cpk::event_data/community.bin::CARD_EF06.EPL` | EPL | 134112 | `cb1ed7c4957fdbd26b0081735cb9b4c0c55587eeb604c5d868c972e738011621` |
| `data.cpk::event_data/community.bin::CARD_EF07.EPL` | EPL | 19440 | `2b55441af548b5b3c90bc1bad8253942d9f68fe7cfa2cdf75779d5f0dcbc86f4` |
| `data.cpk::event_data/community.bin::CMY01_02.EPL` | EPL | 7184 | `4d4eee955d10ab94f0672798d119f60466cb4437c3027c45ad8d13c1cd826b73` |
| `data.cpk::event_data/community.bin::CMY02_01.EPL` | EPL | 7232 | `b5561bee072cbe2532eb2829bf5c39fc91927f53fdf184e65a67b8d01afc5dfe` |
| `data.cpk::event_data/community.bin::CMY04_06.EPL` | EPL | 2816 | `1d99560e14f4d5304bad68a200e428f909d7b806466b1e504b7c8cb1a5b79800` |
| `data.cpk::event_data/community.bin::CMY05_05.EPL` | EPL | 13440 | `4252a6171efe301668d8d50dc48ad1d5157e8fb7ea0e10e1322c31690c4747e1` |
| `data.cpk::event_data/community.bin::E_KIRA.EPL` | EPL | 12560 | `9b83128cbed2ba53cb6c6bea178e6a26ba031a32db7e39f0863e09adac8b5e9c` |
| `data.cpk::event_data/community.bin::E_KIRA2.EPL` | EPL | 12560 | `cbc82cec6fd117a07c16d78d18265390c36c0ec058022e0f36fba5e944fabf85` |
| `data.cpk::event_data/community.bin::P_GET3.EPL` | EPL | 2128 | `928fd9aaecd4c39732a2c2492670dac71de00a6dda594d74c2316286a5127c9d` |
| `data.cpk::facility/cmbroot.arc::E_PER.EPL` | EPL | 1120 | `07b9bac782b287964bc963b576ddc16a1724ce44010e3bf9a86f6972a757a1a9` |
| `data.cpk::facility/cmbroot.arc::GAT07_01.EPL` | EPL | 19840 | `367b85f1f9785cb0a1aace1573b309b9a9eecf9df2d174eef2d78cdc332c8d16` |
| `data.cpk::facility/cmbroot.arc::PER3.EPL` | EPL | 640 | `a8420c4ae86d038a3f45da5fed5afb3bcc040783e1aa5372c1957916fb5bf9b8` |
| `data.cpk::field/CLOUD_A.EPL` | EPL | 560 | `f45c1231ee8df4643c39425485c4648c3607cb260996fc3112f5c97c4cee599c` |
| `data.cpk::field/SUNSET_L.EPL` | EPL | 528 | `679fd68d366e450d037a84a03f45a726ad0062a00e5144e1ed78227d8136b841` |
| `data.cpk::field/SUN_LIGHT.EPL` | EPL | 1344 | `e411b725205d1c83712b9d2cf9e68a86cad7ef323c88ff6e64620d431bd85d32` |
| `data.cpk::field/TRAEST.EPL` | EPL | 52224 | `3c0f7680ed386576649b9acb35d379f826c2d8a3731b3ff9fbe6c8261617f8d7` |
| `data.cpk::field/btl_effect/btl_eff.pac::ENCONT_N.EPL` | EPL | 4320 | `5046b5b8497e11f8f3378fbf9645fbe54adbbece222983043177af88634d41de` |
| `data.cpk::field/btl_effect/btl_eff.pac::F_DAMEJI.EPL` | EPL | 7424 | `8c314b6729abee5756e74f21cb29b2f8abef55831f60674af32948de00a56d04` |
| `data.cpk::field/btl_effect/btl_eff.pac::F_HAKEN.EPL` | EPL | 34064 | `4c108d2c3ff87d5aa4d57ea7e1952095f7b51caed897c47934887f11e153481b` |
| `data.cpk::field/btl_effect/btl_eff.pac::F_ICON05.EPL` | EPL | 11312 | `fafc14c02469d2a535ade388037acaf97808d9ae202601335ab9172bee424acc` |
| `data.cpk::field/btl_effect/btl_eff.pac::F_KAIFKU.EPL` | EPL | 4752 | `525d3c116d057d871c9fc3d792b0243512637e9f15cba3f7e6f6821528ba9a2d` |
| `data.cpk::field/btl_effect/btl_eff.pac::F_MOUDOK.EPL` | EPL | 2800 | `f330455ec2cc5b7d2fd83c0298212d0824e02cc962f0eab71abfbf999bc2990d` |
| `data.cpk::field/btl_effect/btl_eff.pac::F_SAISEI.EPL` | EPL | 15584 | `bfe4645fac8286b004b632cc977b1f18f3e5b18c8dd41675bfb215f12fc4f8f1` |
| `data.cpk::field/btl_effect/btl_eff.pac::F_SIBOU.EPL` | EPL | 5168 | `14e8fd1f4d10c4a6e9d8b6c36c9c66f5fafe714fdbc332bfc7756431cef63fc7` |
| `data.cpk::field/fld_effect.bin::ENCONT_N.EPL` | EPL | 4320 | `6ebb420212ae9deac0e2d658e2bb5f6dfddc9603df68781af87607d82290decd` |
| `data.cpk::field/fld_effect.bin::F_HAKEN.EPL` | EPL | 34048 | `65d3e1687b56740fa5daed2fa5a44326a77a85765c8cdd4173351769904769bb` |
| `data.cpk::field/fld_effect.bin::P_HAKEN.EPL` | EPL | 67648 | `92767ef6908335c8ce8dd11ab4acd54e0041f554ab587dc6197fe8d5157e3b1b` |
| `data.cpk::field/pack/dng_init.pac::ENCONT_N.EPL` | EPL | 4320 | `5046b5b8497e11f8f3378fbf9645fbe54adbbece222983043177af88634d41de` |
| `data.cpk::field/pack/dng_init.pac::F_DAMEJI.EPL` | EPL | 7424 | `8c314b6729abee5756e74f21cb29b2f8abef55831f60674af32948de00a56d04` |
| `data.cpk::field/pack/dng_init.pac::F_HAKEN.EPL` | EPL | 34064 | `4c108d2c3ff87d5aa4d57ea7e1952095f7b51caed897c47934887f11e153481b` |
| `data.cpk::field/pack/dng_init.pac::F_ICON04.EPL` | EPL | 11312 | `ead4ca9611e47cd71bb4506ef73ba8efc48fe22b1dbdefffab6708e596192b80` |
| `data.cpk::field/pack/dng_init.pac::F_ICON05.EPL` | EPL | 11312 | `fafc14c02469d2a535ade388037acaf97808d9ae202601335ab9172bee424acc` |
| `data.cpk::field/pack/dng_init.pac::F_KAIFKU.EPL` | EPL | 4752 | `525d3c116d057d871c9fc3d792b0243512637e9f15cba3f7e6f6821528ba9a2d` |
| `data.cpk::field/pack/dng_init.pac::F_MOUDOK.EPL` | EPL | 2800 | `f330455ec2cc5b7d2fd83c0298212d0824e02cc962f0eab71abfbf999bc2990d` |
| `data.cpk::field/pack/dng_init.pac::F_SAISEI.EPL` | EPL | 15584 | `bfe4645fac8286b004b632cc977b1f18f3e5b18c8dd41675bfb215f12fc4f8f1` |
| `data.cpk::field/pack/dng_init.pac::F_SIBOU.EPL` | EPL | 5168 | `14e8fd1f4d10c4a6e9d8b6c36c9c66f5fafe714fdbc332bfc7756431cef63fc7` |
| `data.cpk::field/pack/f007_002_rmd.arc::F_ICON_AR.EPL` | EPL | 5728 | `b0f16a5f6756f08a38ae37526925c3d558f2b01b1a52baa381c68101d75bedd0` |
| `data.cpk::field/pack/f007_002_rmd.arc::F_ICON_KA.EPL` | EPL | 5728 | `c6a69120e7185325e308a016632f588c13482a1b684a08d7b885e78eb134ec50` |
| `data.cpk::field/pack/f007_002_rmd.arc::F_ICON_OK.EPL` | EPL | 5728 | `ed9cf58284a4c8c3fec4103dca44f33ae2f57b22a66c32d25ddc3d639561dfaf` |
| `data.cpk::field/pack/f011_001_rmd.arc::F_ICON07.EPL` | EPL | 5728 | `61a1881d7193bb7952c9080e16e4275390fd57c3851eb43a53823457e8758afd` |
| `data.cpk::field/pack/fd031_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data.cpk::field/pack/fd031_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data.cpk::field/pack/fd031_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data.cpk::field/pack/fd031_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data.cpk::field/pack/fd049_000.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data.cpk::field/pack/fd049_000.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data.cpk::field/pack/fd069_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data.cpk::field/pack/fd069_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data.cpk::field/pack/fd069_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data.cpk::field/pack/fd069_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data.cpk::field/pack/fd069_003.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data.cpk::field/pack/fd069_003.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data.cpk::field/pack/fd069_004.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data.cpk::field/pack/fd069_004.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data.cpk::field/pack/object.arc::F_BLUR.EPL` | EPL | 640 | `f7c0c9b76c0cc4cb11dbe0816d1d3d83ef72ccd00f33a9480acf0ecd55fcfb6c` |
| `data.cpk::field/pack/object.arc::SNOW.EPL` | EPL | 640 | `d3ddcae1076afbef4c01a36769b307e06cf59f95a49400d5ccd92d2924a4211a` |
| `data.cpk::field/pack/object.arc::SNOW_old.EPL` | EPL | 640 | `ff17b8ecac6518601e522bbc9eaf963fe6b3f480e82167891c228fcf9f8a2a99` |
| `data.cpk::field/pack/object.arc::SUNSET_L.EPL` | EPL | 544 | `76bbb57c8ac5b20c91934c3a212bc4759be160c54287871a6dd98aef4437a450` |
| `data.cpk::field/pack/object.arc::SUN_LIGHT.EPL` | EPL | 1344 | `e411b725205d1c83712b9d2cf9e68a86cad7ef323c88ff6e64620d431bd85d32` |
| `data.cpk::init.bin::event/icon.bin::_ICON1.EPL` | EPL | 35360 | `419957b5504a91136e2a6da0a9191adbb0d0fdcabec7745d7c6a0245337e1a0c` |
| `data.cpk::init.bin::event/icon.bin::_ICON10.EPL` | EPL | 640 | `a459f3d6a49e217485c18351e1be0074790389f619fa11a21c41d10f817a629e` |
| `data.cpk::init.bin::event/icon.bin::_ICON11.EPL` | EPL | 35360 | `a2b85ec8fd28435027137c00eb3db2613488890e426eb15950693e450ae3666e` |
| `data.cpk::init.bin::event/icon.bin::_ICON12.EPL` | EPL | 13920 | `7c600dbf814ca52978843c10998ac5877290426bce5fc15b11d188ced6dc7655` |
| `data.cpk::init.bin::event/icon.bin::_ICON13.EPL` | EPL | 2816 | `4c470d96fa640beb21f4b5c5f69da7278816df340150539b19a6d282b58d3745` |
| `data.cpk::init.bin::event/icon.bin::_ICON14.EPL` | EPL | 3200 | `44511407c6c60dcb3fdffc2ea0b295f3e367373ad4e47fafb8f68ffbfa523742` |
| `data.cpk::init.bin::event/icon.bin::_ICON15.EPL` | EPL | 3776 | `f376c7bd0d6fd7aa6614093b86b47465f5ce418e4456293015687a845851c6c0` |
| `data.cpk::init.bin::event/icon.bin::_ICON16.EPL` | EPL | 8960 | `749535ddb03ac80e9c173a4e2c4e011bb445ef4c3afea446d9327356e03e53e8` |
| `data.cpk::init.bin::event/icon.bin::_ICON17.EPL` | EPL | 5408 | `682ce6151e173a4e5d837be7324c60c7e5475112682d4f7734f32c2200316fa1` |
| `data.cpk::init.bin::event/icon.bin::_ICON18.EPL` | EPL | 2816 | `ac6cae65586ea536872e38741e8d1b4f710faaa61209c23a54c2daf629f38528` |
| `data.cpk::init.bin::event/icon.bin::_ICON19.EPL` | EPL | 10784 | `1712a4d006ad15b02ac959902c2ea753f852832ec6d94c459ca0dd92b2f31ed1` |
| `data.cpk::init.bin::event/icon.bin::_ICON2.EPL` | EPL | 70560 | `d1f15c72717d7ae5b2b5b495c209f9ce6928d98bdd7a0a9e0d45e6c95ba8e12b` |
| `data.cpk::init.bin::event/icon.bin::_ICON20.EPL` | EPL | 11456 | `3c36b0b747116189a73e67773c05f474611512d39db9f7a81c47ce562f40632e` |
| `data.cpk::init.bin::event/icon.bin::_ICON21.EPL` | EPL | 16768 | `ca32d5e7033db22860116d6accd7983ace3ee5c840f8ef936f22566b987bca92` |
| `data.cpk::init.bin::event/icon.bin::_ICON22.EPL` | EPL | 4448 | `d0d9ad27c09e15df5caccb17467b71e5b15ba39d20f2c325d4d445caa7aac979` |
| `data.cpk::init.bin::event/icon.bin::_ICON23.EPL` | EPL | 7328 | `83a7d4ae49cc4d9237798ed7eb905ef31e237550537bd89f67cf5744fbf8d102` |
| `data.cpk::init.bin::event/icon.bin::_ICON24.EPL` | EPL | 13440 | `91538d6b1c7770ad8a4b4a2c34556939c9c1e4f1fc1beede20f8ba87ac5d5e7a` |
| `data.cpk::init.bin::event/icon.bin::_ICON25.EPL` | EPL | 52960 | `5d8963dec880d512cc7453b46e2c0187e4113a21f82a431656472a48467951e5` |
| `data.cpk::init.bin::event/icon.bin::_ICON26.EPL` | EPL | 2816 | `37621fb4cae82626436d81fabdc9b4177905063ac5264703e7de25f58e06bad3` |
| `data.cpk::init.bin::event/icon.bin::_ICON3.EPL` | EPL | 12928 | `c8174ba08c05c1d7c62cb57138c3487e376dcf16713d3341a6018fc413e46688` |
| `data.cpk::init.bin::event/icon.bin::_ICON4.EPL` | EPL | 32928 | `2f173d83c1e9a766c6bd5f21b90812ba376f44e2c7fd4b314b3e8c3004087b64` |
| `data.cpk::init.bin::event/icon.bin::_ICON5.EPL` | EPL | 10464 | `72c96b08b6fdf5fcc5aad1a3dc4961b30e063647d1c8bac2a51ec464789c7bdd` |
| `data.cpk::init.bin::event/icon.bin::_ICON6.EPL` | EPL | 24064 | `9115e4344e6efa200a9b2bad425236d6c514a6f4109e1154b080140c0889cd1b` |
| `data.cpk::init.bin::event/icon.bin::_ICON7.EPL` | EPL | 2816 | `dedee54a4f210c6a3e993d17ed78bbc896a6c1e97acafc1e50e21b7ceb78f149` |
| `data.cpk::init.bin::event/icon.bin::_ICON8.EPL` | EPL | 5472 | `15038b74e6204b01c8f1fee6595dba30b926e4a37e052bc8de8f53b10fb77dec` |
| `data.cpk::init.bin::event/icon.bin::_ICON9.EPL` | EPL | 34688 | `f4970730444758d9b1e2c20933ecdba4d9c69a92422d4722eb33caf384c0491b` |
| `data.cpk::init.bin::field/F_BLUR.EPL` | EPL | 640 | `f7c0c9b76c0cc4cb11dbe0816d1d3d83ef72ccd00f33a9480acf0ecd55fcfb6c` |
| `data.cpk::init.bin::field/F_ICON01.EPL` | EPL | 11856 | `bb0d1adabff9e8808efb526a1d5ed9a438e00209958956d7fd8b83d811836882` |
| `data.cpk::init.bin::field/F_ICON02.EPL` | EPL | 11312 | `37486846f11641eae744873620fd47689a2b343adebd53a567287c1918007555` |
| `data.cpk::init.bin::field/F_ICON03.EPL` | EPL | 11952 | `9f58f431f3147529befe48618a378f464538847caf9d1ac7ec6cf0fb2a1c21c7` |
| `data.cpk::init.bin::field/F_ICON04.EPL` | EPL | 11312 | `a2b0a94439e8716ecece6426d24874563ad7d39294abd923b21c226ca890eeb7` |
| `data.cpk::init.bin::field/F_ICON05.EPL` | EPL | 11312 | `24aa952f33209549dd9a2c0ed01f2bf1f9e1f3d1883fcd1bdcdca959d4c827ca` |
| `data.cpk::init.bin::field/RAIN_01.EPL` | EPL | 9440 | `af7e203dbc36d37d53577a673f20b9a6b71fa5bd1b52189326d4ea476e560c5e` |
| `data.cpk::init.bin::field/SNOW_01.EPL` | EPL | 640 | `4c614a3d103ad5745d5610865f9e2cdac2fc4b90c04b343d1f3ff61401c0bba1` |
| `data.cpk::init.bin::field/SNOW_02.EPL` | EPL | 640 | `3f025061842c24fa748cdd09adabd7de493c95eae332e544557efd0602798454` |
| `data.cpk::init.bin::field/effect/BREATH_01.EPL` | EPL | 640 | `9e5da14b05e6803b490c3f6e71ceac80965aaba7184b3ce317229ecf53617630` |
| `data.cpk::init.bin::field/effect/FOOT_L.EPL` | EPL | 5728 | `e4169139debccf8f28e88e86e2b72a49e10ecfee15a61eb745fd1a1bf1f18eee` |
| `data.cpk::init.bin::field/effect/FOOT_R.EPL` | EPL | 5728 | `83828867762a5b9ed87f11eb45f91284974350052e85d8993a737fc0229e12a2` |
| `data.cpk::init.bin::insect/insect.bin::INSECT_01.EPL` | EPL | 9696 | `74be2671465a4e5e760c6697cd27f5e96dce151f48b488fc40262c75cda40ea7` |
| `data.cpk::init_free.bin::battle/effect/SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data.cpk::skill/000.pac::000.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/001.pac::001.BED` | BED400 | 88960 | `1566f916cf1e783561c6b025bcf5733a144d05c99b222651172aaefeb3f06f86` |
| `data.cpk::skill/002.pac::002.BED` | BED400 | 38240 | `4d2354d7a8c0ef08ad2cb80330bd755e89025bac58320d4996c1ce22181ea1ca` |
| `data.cpk::skill/003.pac::003.BED` | BED400 | 47488 | `b47050a3bc2f1062472e49544a33d860d29a1fd275d093ede84fd92025c204a9` |
| `data.cpk::skill/004.pac::004.BED` | BED400 | 87584 | `be354c81285c25d3b363100b672b80b31fc4cc6ec83cbdf84d71ad4d8e9e9518` |
| `data.cpk::skill/005.pac::005.BED` | BED400 | 36288 | `e54c2a04a5859a94d9e29f7c174632ddb5edf7dabb886f9613216a7b7d66e5f8` |
| `data.cpk::skill/006.pac::006.BED` | BED400 | 46624 | `38983d2591a52dd0a5c06956e7aca2dc5e004cd1550dc2c9860f1124b763318b` |
| `data.cpk::skill/007.pac::007.BED` | BED400 | 90400 | `7643ae605f6d99695a97e7ee45800f5ba42e1ab90ad3d5c44d8a00efd786873d` |
| `data.cpk::skill/008.pac::008.BED` | BED400 | 9024 | `75d31907c385131adb863620636612c5d54db2b49eb6c690d98c07887328ba6d` |
| `data.cpk::skill/009.pac::009.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/00A.pac::00A.BED` | BED400 | 43104 | `8a9fd7837e57bb90ab276e9fc2ea3d03a0df9ba6d0a8902d98291e89c8b49991` |
| `data.cpk::skill/00B.pac::00B.BED` | BED400 | 33120 | `10121e63b88a19ed6eb56d43bf9b7339c42025269b374d83c9a1a541b047b7aa` |
| `data.cpk::skill/00C.pac::00C.BED` | BED400 | 29408 | `15ebd2527f9ee6f69b2ee5b6d3d8ce0244274e663593dba02066b12fc8b81133` |
| `data.cpk::skill/00D.pac::00D.BED` | BED400 | 29408 | `45cb9550a3dc0468e5a0c0d21bb146a91d1fb7cd65c6385481749a69c8a0a68e` |
| `data.cpk::skill/00E.pac::00E.BED` | BED400 | 30784 | `8dd5180f64b8cce5cb222fa634fbea136316f2359c353008769c9581f49ebd11` |
| `data.cpk::skill/00F.pac::00F.BED` | BED400 | 37184 | `3b1789fa8d9b77b8b68d887c3c2da00b989c5f31697dc8b3a5392301c0f3bc49` |
| `data.cpk::skill/010.pac::010.BED` | BED400 | 168192 | `5ea1b17412d196b2ff01202752dc4fb7798fd4c3cc2d4af6b028c8ba8951e8c3` |
| `data.cpk::skill/011.pac::011.BED` | BED400 | 29632 | `35fcd4b3375fcd50e51d69b846a6ffd9831005394f2f47f7d2bcb38ab88ccbc5` |
| `data.cpk::skill/012.pac::012.BED` | BED400 | 33216 | `25506fbc0114ca3616c9b8283219feb19de1606514db4ee0fe2d8b9b01e25de6` |
| `data.cpk::skill/013.pac::013.BED` | BED400 | 156832 | `eb8912d41fea401360e6ae572d4d0c2aa1c0732f1141919667793c0f1e5df322` |
| `data.cpk::skill/014.pac::014.BED` | BED400 | 142464 | `781a11b2b147a92ba28f76b5fa89c44a17f8c93b7b8c802a51a9cdf11c6ea4f6` |
| `data.cpk::skill/015.pac::015.BED` | BED400 | 184896 | `d28c4ce41eb28299b155ccbf3a476486b0287c2da11bae5047d3ee32568c7531` |
| `data.cpk::skill/016.pac::016.BED` | BED400 | 78880 | `ab1190046299d3f5e78c6ce9a0fcb83b6fa4a14fa753d14fc4ea7a66f7b80421` |
| `data.cpk::skill/017.pac::017.BED` | BED400 | 8704 | `2ebf56a082883cfdb658f6dd4f2400e428eb5ca38b5bf5d4f360beca64608649` |
| `data.cpk::skill/018.pac::018.BED` | BED400 | 99520 | `7f85ff99fe2f0e14c801a5c1bbc5cc9af42be1136b23bdf862c78e250a5b239f` |
| `data.cpk::skill/019.pac::019.BED` | BED400 | 117728 | `ffcb74ee464e357970038d2901b6b52d3414546022ea80ebedbbf0f2da9b620a` |
| `data.cpk::skill/01A.pac::01A.BED` | BED400 | 160480 | `7eeb1a3a2a1cd5662b95fc33c68150db71874c8fc5aa2577591a2c006c95176c` |
| `data.cpk::skill/01B.pac::01B.BED` | BED400 | 57152 | `7ab5700e472737e0ced96569a30eac1e7d123419a4158b893a4a78a5bbe8e696` |
| `data.cpk::skill/01C.pac::01C.BED` | BED400 | 101696 | `9ef911ea32cfd7bbf8a98638906483c0f01e3b658cf911e55ff6942256b5e75c` |
| `data.cpk::skill/01D.pac::01D.BED` | BED400 | 160640 | `797e10c5c3dddd1a7e505670b5d34a6ac4608fd52092f8dbb046e0ca2b72ee95` |
| `data.cpk::skill/01E.pac::01E.BED` | BED400 | 55328 | `ad8e13e455c77bcb0499fd05a693e34528a361de8a4b312e462d9f91cfc2ff8d` |
| `data.cpk::skill/01F.pac::01F.BED` | BED400 | 101824 | `58cce569901d2377dbad1863c192798aa91375f08c5763bb7ddb6c0e464c3f39` |
| `data.cpk::skill/020.pac::020.BED` | BED400 | 26528 | `9d2c3bdb031d766c2522955d137b650966a51e8bc1895061b596115b620b832f` |
| `data.cpk::skill/021.pac::021.BED` | BED400 | 32928 | `e09fd7eaf079f2e82468d405d8b062863364d674f1945f18865b93d5eb51b0a7` |
| `data.cpk::skill/022.pac::022.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/023.pac::023.BED` | BED400 | 60064 | `a5823ec62e4b85226a87a0f5f9c74fbf94afe5af09c622b82be626f8a1bab16d` |
| `data.cpk::skill/024.pac::024.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/025.pac::025.BED` | BED400 | 99520 | `5941ef21fecdd3d07fb1f5ea425b8852743853741f42795edb193b1fb5454866` |
| `data.cpk::skill/026.pac::026.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/027.pac::027.BED` | BED400 | 15584 | `84d33e79e474e1aad5a7830863137795d9a02ce33534f573315ee92585635c23` |
| `data.cpk::skill/028.pac::028.BED` | BED400 | 33856 | `bdd2e3621e7f311ebe987eb4bde9df4cecd907e361a27f6774f4cbc528bb3767` |
| `data.cpk::skill/029.pac::029.BED` | BED400 | 109504 | `968ec49f1149849058ffd9cba6ea9c250271c707b6e1fd60f974b6c679183765` |
| `data.cpk::skill/02A.pac::02A.BED` | BED400 | 15104 | `7aa8074e0640fbdd3b767e021dd5507ee9f9b6bc0f0c470aaa0fc65adff3dfd2` |
| `data.cpk::skill/02B.pac::02B.BED` | BED400 | 26368 | `a5db3291c8478314e5a21e9c841922c924c407808897c1052d08495c935a7904` |
| `data.cpk::skill/02C.pac::02C.BED` | BED400 | 101856 | `9a3b51a0dfe4de6271891f8f955ffa07499e7e60db161deb88edef0fc6bc19a2` |
| `data.cpk::skill/02D.pac::02D.BED` | BED400 | 99520 | `b01c3fe33c079424e92f65ec7b762e83230055e51ff1c1083209a5eaab966ef1` |
| `data.cpk::skill/02E.pac::02E.BED` | BED400 | 178016 | `cc228d7621d83c0279f19a917d36d844b73f04284692852a155107ef14016e20` |
| `data.cpk::skill/02F.pac::02F.BED` | BED400 | 78880 | `ab1190046299d3f5e78c6ce9a0fcb83b6fa4a14fa753d14fc4ea7a66f7b80421` |
| `data.cpk::skill/030.pac::030.BED` | BED400 | 78848 | `739040f11af537dda61757f1179ad1cf500d362846181d5f21abb5217dc77c7c` |
| `data.cpk::skill/031.pac::031.BED` | BED400 | 79968 | `824cda2410fe945bf4908b1870ddc3b8a27d364fb4db2acaa98ea6ed9e249e84` |
| `data.cpk::skill/032.pac::032.BED` | BED400 | 33120 | `66428542983883a6bf6864733c88106088bed9f48daf763bf5dc93150e5d9eed` |
| `data.cpk::skill/033.pac::033.BED` | BED400 | 9728 | `a984841402522806c228c6c9e86365a45e33a97607b6afabbc3362383b260c6d` |
| `data.cpk::skill/034.pac::034.BED` | BED400 | 40832 | `553d1e8390d2c4562e8c1fa8f2372d73693ca8bb46b77aeb5104a51ef2df0443` |
| `data.cpk::skill/035.pac::035.BED` | BED400 | 43104 | `461a346ea8eb595f74d921db754414a6f792f2a3afefc1c7b08873504c3bc17e` |
| `data.cpk::skill/036.pac::036.BED` | BED400 | 78016 | `c1a0468b9df4b351da9d7765e306f41526ce98090cfd2169bb016d00cd8d26ea` |
| `data.cpk::skill/037.pac::037.BED` | BED400 | 122816 | `5c57169c0859505abc5e76fa65cfc35163e09ec2acb1bdc00aa0f744abeb7ba3` |
| `data.cpk::skill/038.pac::038.BED` | BED400 | 179264 | `a948117101d984a626da3c487ad31b03aa58064796c332975c0a69bef51d411c` |
| `data.cpk::skill/039.pac::039.BED` | BED400 | 87744 | `370bce2f3d909638df02ec4800faee5ebda77c5403f50847175d5b45a5283896` |
| `data.cpk::skill/03A.pac::03A.BED` | BED400 | 87520 | `3837adcd3ab4e50d53400edd32cbf49b9dc38cf64ac9cffa24a6f37ce4a0ff72` |
| `data.cpk::skill/03B.pac::03B.BED` | BED400 | 70592 | `26f9734531c3dd19f555a11ccc2c8534c5a76fb866232200bb95eba626e9badf` |
| `data.cpk::skill/03C.pac::03C.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/03D.pac::03D.BED` | BED400 | 79168 | `dbbd2e38a316e6ad0ae12c9fda398cec7b7e40fee0a67657d9715378a7aa9038` |
| `data.cpk::skill/03E.pac::03E.BED` | BED400 | 178016 | `2082f625fdc6271ec28bef027bbcb6bcededd57ef4cf882c336a611268db360c` |
| `data.cpk::skill/03F.pac::03F.BED` | BED400 | 340000 | `9c82f1212e0f9c84343e411afcefa0558db4a87f9e5a415bc12e77f36339d264` |
| `data.cpk::skill/040.pac::040.BED` | BED400 | 202816 | `36146ecd747879d9afc171a98ce20c52b88784c52536fb1ef32b5d99327de49c` |
| `data.cpk::skill/041.pac::041.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/042.pac::042.BED` | BED400 | 54816 | `7839c6bee2d864de67627c5a51157e9e9e251215722e0c89ee0aad0113fc673c` |
| `data.cpk::skill/043.pac::043.BED` | BED400 | 181184 | `0ed9d26eec37c2629cc3ebdfc2105295d0c9fd425a8064542f3fc1745ba11d1f` |
| `data.cpk::skill/044.pac::044.BED` | BED400 | 84320 | `28f99ce0d9ccae71f83d2c6ea6040a9c0cbfa39a0e7dd7dcd6dcc27a5e19b208` |
| `data.cpk::skill/045.pac::045.BED` | BED400 | 86464 | `26bb572f51a0ed45aa7c74456c581f6f58e7a528e0c9b56e567620be05836c02` |
| `data.cpk::skill/046.pac::046.BED` | BED400 | 98528 | `d6c8eed803f8a385381dc25dc19b5d0f6e9892e95badc401ed707fe6b75c180e` |
| `data.cpk::skill/047.pac::047.BED` | BED400 | 149504 | `fd3ceb54ea544897067085a718363c9ece11ab3bf9aecbfd1d59a5e1ef9342d4` |
| `data.cpk::skill/048.pac::048.BED` | BED400 | 77632 | `8495e571563094aaaa45308990d769b0f74c5e477ae05847d6139e7c0863b728` |
| `data.cpk::skill/049.pac::049.BED` | BED400 | 98112 | `5f567ed9fae13772ea5bc7d3896fbc5b5eab3c9f0f2ed48fe33c4a9e2bfa7e33` |
| `data.cpk::skill/04A.pac::04A.BED` | BED400 | 89216 | `aff5a31bf7e2d80375bb1f4cd3564667038ff42c642a697dfa19c904e121c2da` |
| `data.cpk::skill/04B.pac::04B.BED` | BED400 | 146464 | `49929e7c268265329f32ef57709f5aa5eb9792494f12ccf7e8b833c5f774b44f` |
| `data.cpk::skill/04C.pac::04C.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/04D.pac::04D.BED` | BED400 | 80288 | `dd87d9d9a716b42407569bbaf99704ad138bc37cfc84d028e286eacd5bdd24bf` |
| `data.cpk::skill/04E.pac::04E.BED` | BED400 | 187488 | `ebd3973bf054421dab8d87a747689eaca8053911b50b9244db3152a8411729cf` |
| `data.cpk::skill/04F.pac::04F.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/050.pac::050.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/051.pac::051.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/052.pac::052.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/053.pac::053.BED` | BED400 | 28352 | `8cd331b0247ea3143e571b8a8f05fbfdc4a492a4d69e7315c3889fea080c64af` |
| `data.cpk::skill/054.pac::054.BED` | BED400 | 13728 | `925b5ddf5e7029e2dbd902cc5eea8d32ca548d0b46da68a6a213b0f1fe775627` |
| `data.cpk::skill/055.pac::055.BED` | BED400 | 50048 | `c453806f92c1b27e2dae2a31d137cef34bfd9e6abd3e300104af06eabcd76351` |
| `data.cpk::skill/056.pac::056.BED` | BED400 | 29152 | `4759cc3b3d3025ad0b18c43cafe1d0d6ddbed751b19625aba9d7a8bba2774f0a` |
| `data.cpk::skill/057.pac::057.BED` | BED400 | 53952 | `19ec8d69b7504c98fe59e36ca7798fb5c443af3ef5a3bf1fac502104fe767af8` |
| `data.cpk::skill/058.pac::058.BED` | BED400 | 9664 | `6960c32d49167631b9ec5f313a57a8d195d976cb652c7c662265de5e787b72f3` |
| `data.cpk::skill/059.pac::059.BED` | BED400 | 8800 | `7762b3d3b613a3e4055db25183f89cce686b435a1d679d88cd3f03f39385d1c3` |
| `data.cpk::skill/05A.pac::05A.BED` | BED400 | 38304 | `4fda1efe35a5371215d6c84f2c781fcf42c8a6ab191062d41438ff32bc8671a1` |
| `data.cpk::skill/05B.pac::05B.BED` | BED400 | 10208 | `e5b4f45fa2d35cdca12745c54f2118560d77b7a9d13c0c6c9dc82c369214579a` |
| `data.cpk::skill/05C.pac::05C.BED` | BED400 | 68576 | `fe7b1a830ab7575c6166d069bb6e3ec20e9fed401c4c6727d2336895cccee000` |
| `data.cpk::skill/05D.pac::05D.BED` | BED400 | 66432 | `53a159761133995ec1856f6475d5a462831df1970ad6f9c8b5133e5d0581c420` |
| `data.cpk::skill/05E.pac::05E.BED` | BED400 | 50048 | `64a72e7aa3e77ade3c029295cba963c29e72e399b0ef37e20d849680ef1f7948` |
| `data.cpk::skill/05F.pac::05F.BED` | BED400 | 81632 | `0b2089a6dd705a34a03a576aaef1efe4f0a6c2768d88951c67f7957ca12abf93` |
| `data.cpk::skill/060.pac::060.BED` | BED400 | 186048 | `b18762f85c473afe933df0e95c8b265803266357728ad229a882d6b5e24d2908` |
| `data.cpk::skill/061.pac::061.BED` | BED400 | 83200 | `81365bdbc06c0abfc0afcf076ea74858d251acd02fdb0cac57b665702ad8a235` |
| `data.cpk::skill/062.pac::062.BED` | BED400 | 97056 | `445af55491cbbdd7d0dfff5f98fd845eae32efd09229e2c665523270c607c136` |
| `data.cpk::skill/063.pac::063.BED` | BED400 | 38656 | `a6d06f429946caf6bc2ee5afb019720c0f8e9180b093959b12b632e804416ed6` |
| `data.cpk::skill/064.pac::064.BED` | BED400 | 12960 | `618c3d1380619e8e2f958b683558226106672d067f15242dfe7599cd9b80b06a` |
| `data.cpk::skill/065.pac::065.BED` | BED400 | 11808 | `1eac88dd92ae5c379bd0291ebd1d8f6463753e0a3e738518eb0c8f5905d21814` |
| `data.cpk::skill/066.pac::066.BED` | BED400 | 11808 | `3b892c6b8bb8a70b270a48dd04c67674c85410bf17171ef93614e0812233b48f` |
| `data.cpk::skill/067.pac::067.BED` | BED400 | 19648 | `aa7f0fc965cdced100224508987ba77fdd12262b4d793bb991895c3b55fb194c` |
| `data.cpk::skill/068.pac::068.BED` | BED400 | 19648 | `49195f033e8f219d990f009f9d86009c852e2e8d8ecfcc3e437918586e5c01ad` |
| `data.cpk::skill/069.pac::069.BED` | BED400 | 285920 | `21011e4d4b6e8d8216c95a15adc86b04a8417be33942fa125b50b6439c7247e0` |
| `data.cpk::skill/06A.pac::06A.BED` | BED400 | 9664 | `aa85c94fcf3adc5f94ffb703cb44eff7ec4b43c674b18712e0dc6634cf015b37` |
| `data.cpk::skill/06B.pac::06B.BED` | BED400 | 78016 | `c1a0468b9df4b351da9d7765e306f41526ce98090cfd2169bb016d00cd8d26ea` |
| `data.cpk::skill/06C.pac::06C.BED` | BED400 | 78016 | `579ff144ea8b23273fa7f1066546e7719401aaa92d810eddd0dd2fdf42e8bea8` |
| `data.cpk::skill/06D.pac::06D.BED` | BED400 | 201696 | `64a8657f1f51e05aaa1f8122f9912b71aa0a801054a2ff6761d55e8700546c15` |
| `data.cpk::skill/06E.pac::06E.BED` | BED400 | 124864 | `112667019ecad082ac7c5a16b28cac49186e703fd8cd8b7561a176d70ca3d443` |
| `data.cpk::skill/06F.pac::06F.BED` | BED400 | 124864 | `112667019ecad082ac7c5a16b28cac49186e703fd8cd8b7561a176d70ca3d443` |
| `data.cpk::skill/070.pac::070.BED` | BED400 | 124864 | `112667019ecad082ac7c5a16b28cac49186e703fd8cd8b7561a176d70ca3d443` |
| `data.cpk::skill/071.pac::071.BED` | BED400 | 124864 | `77ea29f60e73011c8aa99df133190833fcea5904028bfd1d23d599890fc07868` |
| `data.cpk::skill/072.pac::072.BED` | BED400 | 124864 | `112667019ecad082ac7c5a16b28cac49186e703fd8cd8b7561a176d70ca3d443` |
| `data.cpk::skill/073.pac::073.BED` | BED400 | 124864 | `112667019ecad082ac7c5a16b28cac49186e703fd8cd8b7561a176d70ca3d443` |
| `data.cpk::skill/074.pac::074.BED` | BED400 | 124416 | `0bf85a9c8dc3c6a838b7c1874052be99e7634a9563871b7063df18fad9409940` |
| `data.cpk::skill/075.pac::075.BED` | BED400 | 124864 | `112667019ecad082ac7c5a16b28cac49186e703fd8cd8b7561a176d70ca3d443` |
| `data.cpk::skill/076.pac::076.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/077.pac::077.BED` | BED400 | 84320 | `28f99ce0d9ccae71f83d2c6ea6040a9c0cbfa39a0e7dd7dcd6dcc27a5e19b208` |
| `data.cpk::skill/078.pac::078.BED` | BED400 | 50176 | `9cb8819d0294130a79280b9328035b1e6901b4b035ae53786c3d9f35b6168351` |
| `data.cpk::skill/079.pac::079.BED` | BED400 | 14752 | `6ef8081fd0612a81601a2d41d422600c38876fd77765503a93a01665a58828d7` |
| `data.cpk::skill/07A.pac::07A.BED` | BED400 | 45312 | `9418be7d80c9176aa291b390c8e3d96fb60ad3369919ed973f601d9e9f315b01` |
| `data.cpk::skill/07B.pac::07B.BED` | BED400 | 28512 | `7cd334754a820f97688cc1db9910e97352b6e1de44d33199cd56baa46f83e7c7` |
| `data.cpk::skill/07C.pac::07C.BED` | BED400 | 47872 | `cb66f218f2fdcc66ee4f107793272ca81efca22bdc787a89eff4bd36aaf25b4f` |
| `data.cpk::skill/07D.pac::07D.BED` | BED400 | 17184 | `2bc44ce510f1da10d16a9b9746b1107dc070b52524f477b5216e277f5b1dc7e0` |
| `data.cpk::skill/07E.pac::07E.BED` | BED400 | 48288 | `be78582b32004ae0f8f32ff39b17229117df370a8283438bd4f1bc100e290dd7` |
| `data.cpk::skill/07F.pac::07F.BED` | BED400 | 118240 | `5658b05a81817c3d3ef865ff4e405d568f79843e351e17d964c70f6ef5ee806f` |
| `data.cpk::skill/080.pac::080.BED` | BED400 | 24736 | `5fd9ed9fe558ab556a254e16ce3e5b522e6f9726ee74c7221d8d5a4b98d29153` |
| `data.cpk::skill/081.pac::081.BED` | BED400 | 90848 | `1457e5c12963b1170a76afbc6ab34e002b4250528fd8f3a1af880175904ffa29` |
| `data.cpk::skill/082.pac::082.BED` | BED400 | 54752 | `ff894d3eb5944626d43969213eff8ffff68ec5b9bf3f1094e1c6137b43bd3cdc` |
| `data.cpk::skill/083.pac::083.BED` | BED400 | 18912 | `c2df76245467d859840cfb26977bd240c62a63fb086a64e0308eec84816ac205` |
| `data.cpk::skill/084.pac::084.BED` | BED400 | 26848 | `51fab21b885ac07efb940d7ec420d4d028aa049c1caced69b73d9f6d76098af5` |
| `data.cpk::skill/085.pac::085.BED` | BED400 | 64160 | `e2fe67dbed9d537d4067a122623a9a282b8745a2186e52d7056c3356109055e9` |
| `data.cpk::skill/086.pac::086.BED` | BED400 | 17152 | `d96bb25804ad6496e69d9e83d08bfc62f019a312c9cd9d15828d932bc7d90523` |
| `data.cpk::skill/087.pac::087.BED` | BED400 | 36672 | `d1dae02f6b9c55b2d35d9ba64771deac42694d073b1d0e6f41a7249b89c72463` |
| `data.cpk::skill/088.pac::088.BED` | BED400 | 30272 | `b13947169035d6ff69754437f761e11c32d3def6b99ca7fd2c613b432e5a1fcc` |
| `data.cpk::skill/089.pac::089.BED` | BED400 | 97376 | `40a105ae29776a46ddd31ebe36ec8cc7b97592f59661c48c42a5618d81517af2` |
| `data.cpk::skill/08A.pac::08A.BED` | BED400 | 88480 | `8f0ca60d7774178627e3eb514ebc89aab8463c1c6dd862f10857ad2c70300fc7` |
| `data.cpk::skill/08B.pac::08B.BED` | BED400 | 23680 | `1cf433cbfe4d576c5a135c6282ef06997141ec04efcb3a91229d91cf5cf956e8` |
| `data.cpk::skill/08C.pac::08C.BED` | BED400 | 83904 | `08b7dcb4bec8de9449060d11f7ac32b23e1dbf7cb421943e5c14aba91ea72e3c` |
| `data.cpk::skill/08D.pac::08D.BED` | BED400 | 129856 | `b5305bbd8432e6ced4ff98fa24d2bd7e8c8732c8e70097350ae0c512feda2c15` |
| `data.cpk::skill/08E.pac::08E.BED` | BED400 | 45408 | `2b7d4a816cbcad11c5a9b27b1216c4f7958ec6b984d694ff5070b407fe220cac` |
| `data.cpk::skill/08F.pac::08F.BED` | BED400 | 55200 | `91b72bda193b0ca1433a81a37f7a1c73ae4f29767b9134d5fb8859efff34467c` |
| `data.cpk::skill/090.pac::090.BED` | BED400 | 15520 | `29f2cf95bca60b2335fed46c42e5018b956aea31b26d332eed4a35fb20ff67df` |
| `data.cpk::skill/091.pac::091.BED` | BED400 | 64320 | `6c86f43af2fb4c99dd109befc928177efd9deb52d04d1c0fde78aa660cce957a` |
| `data.cpk::skill/092.pac::092.BED` | BED400 | 111360 | `56c2a2f02835f02e755f990294c8dae28f0e133bc14857d5daa9c20130f3b478` |
| `data.cpk::skill/093.pac::093.BED` | BED400 | 18592 | `179082ce821aa49586abd0de7895f024c69dcc39e9d2101c99a140e956e09020` |
| `data.cpk::skill/094.pac::094.BED` | BED400 | 12256 | `86ffd56ab7c5373cb59b020bed2bc8f8479d4bab0f5ae6ce80839e5d9646c591` |
| `data.cpk::skill/095.pac::095.BED` | BED400 | 61664 | `e4239b85c482bdc7b061263f7e8971c3f49cc42e4a0d8bdf5a97c414c34f58b9` |
| `data.cpk::skill/096.pac::096.BED` | BED400 | 184224 | `dcaf0db8e9302b1de5f3575d27c31896c4e5ccbe321286d4cd4f9182a977ba3f` |
| `data.cpk::skill/097.pac::097.BED` | BED400 | 183456 | `851311e2f5efc48b1d995d0b2c6d3490638bd328297562b7050d26081881d894` |
| `data.cpk::skill/098.pac::098.BED` | BED400 | 54752 | `ae09a2983559a0059495987cb05a29fa3ffa1e7ee78f9c34e1c945596d156362` |
| `data.cpk::skill/099.pac::099.BED` | BED400 | 118368 | `70d1a42c72390d594133a431277a30cd6b3c08b44aa101494062553748d9426c` |
| `data.cpk::skill/09A.pac::09A.BED` | BED400 | 53056 | `65dbda9d04f493c04e959ac63828592e7bbcf63e83668e1df35cc833d458ac96` |
| `data.cpk::skill/09B.pac::09B.BED` | BED400 | 7776 | `9ebf9c06377ed00798513fac83a8698f7b3753b275b12b43ed4e197b76cd0bce` |
| `data.cpk::skill/09C.pac::09C.BED` | BED400 | 147808 | `e9843b1c2681b9027a74535f71640e29459bb9e1845315d526b386b0127e0237` |
| `data.cpk::skill/09D.pac::09D.BED` | BED400 | 80608 | `c75ba5b330481125e2f993fd0794c25b45e98644bc6c7f75af9fd8f1068dd78f` |
| `data.cpk::skill/09E.pac::09E.BED` | BED400 | 92320 | `aed99b3b53c8f02c69a7b7c2fe55f1ea7229a18ee129ff9b1699f41cc8496420` |
| `data.cpk::skill/09F.pac::09F.BED` | BED400 | 32800 | `6952bda6a140f455e5382decaa0b4eab7f9191bb2f9059ba7822af8fdcc925c6` |
| `data.cpk::skill/0A0.pac::0A0.BED` | BED400 | 52640 | `5c51f3288054ff23554abc26012caca14595bec4240b4f538c1c26259569d520` |
| `data.cpk::skill/0A1.pac::0A1.BED` | BED400 | 44704 | `f1d8af846ca7e63f8851c29689fb03da1ebc3fbb8797ffffc520f695e6514ab9` |
| `data.cpk::skill/0A2.pac::0A2.BED` | BED400 | 182720 | `1f0794c5c0cde6559204c7943049c14d329b9931ce55cc0ced1113ce5b40b87e` |
| `data.cpk::skill/0A3.pac::0A3.BED` | BED400 | 42048 | `31d440bac1ad6e0704525254b04c4bc81701f0b6889747dae33feafc992e9ac6` |
| `data.cpk::skill/0A4.pac::0A4.BED` | BED400 | 59008 | `958abb0dd3403f233574f443a1966b6943aa56c62805cfb8727346f93ff3ea07` |
| `data.cpk::skill/0A5.pac::0A5.BED` | BED400 | 165248 | `b7d32dcf66748d8629802db24809da1f76a208ef2a913761eb6848bbe5981d81` |
| `data.cpk::skill/0A6.pac::0A6.BED` | BED400 | 144608 | `ed797265eb484284999819737a70e6ecc4af952f9f4a98a0a0da76408da8e71d` |
| `data.cpk::skill/0A7.pac::0A7.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/0A8.pac::0A8.BED` | BED400 | 47456 | `506e1d8e5db4986668ffc0124e449966fc70554be640e95066f2555955435ce5` |
| `data.cpk::skill/0A9.pac::0A9.BED` | BED400 | 146304 | `6a75bc1e606a8dbdb60f11b66a34b7218af3f5f332fe1096ff81ce9805b01b27` |
| `data.cpk::skill/0AA.pac::0AA.BED` | BED400 | 57088 | `eb70e7442b4dd1b2cbd1450ee23c291be813e90f658c5efb4edab8a1bc548dc2` |
| `data.cpk::skill/0AB.pac::0AB.BED` | BED400 | 60896 | `21e7540253aea2ee2b26f06258da6c984f7fac56e677d794603ba5a21ba8d548` |
| `data.cpk::skill/0AC.pac::0AC.BED` | BED400 | 56128 | `fdd5b8fe12eb3bcdc069a0e8cabd8ec8316ccb973958d19f2e3cd2f237b2e909` |
| `data.cpk::skill/0AD.pac::0AD.BED` | BED400 | 60576 | `2a94902a458d087dae4296168371f3cdc181b82e8980b6d6a6e77924a1fb39eb` |
| `data.cpk::skill/0AE.pac::0AE.BED` | BED400 | 70240 | `7db534bba04b27cd06f47ba55a7123bb4a764473c6117949e15265092553e83a` |
| `data.cpk::skill/0AF.pac::0AF.BED` | BED400 | 70016 | `1714be471017953fbf218b5b98394090b8a10ebdbb225505470eb70446af5b95` |
| `data.cpk::skill/0B0.pac::0B0.BED` | BED400 | 60896 | `21e7540253aea2ee2b26f06258da6c984f7fac56e677d794603ba5a21ba8d548` |
| `data.cpk::skill/0B1.pac::0B1.BED` | BED400 | 2240 | `f90a0966bae9b4bdf61088f4a20bd7e5f8aba54f12269fb5a6327e0672b28ae0` |
| `data.cpk::skill/0B2.pac::0B2.BED` | BED400 | 2240 | `f90a0966bae9b4bdf61088f4a20bd7e5f8aba54f12269fb5a6327e0672b28ae0` |
| `data.cpk::skill/0B3.pac::0B3.BED` | BED400 | 2240 | `f90a0966bae9b4bdf61088f4a20bd7e5f8aba54f12269fb5a6327e0672b28ae0` |
| `data.cpk::skill/0B4.pac::0B4.BED` | BED400 | 2240 | `f90a0966bae9b4bdf61088f4a20bd7e5f8aba54f12269fb5a6327e0672b28ae0` |
| `data.cpk::skill/0B5.pac::0B5.BED` | BED400 | 2240 | `f90a0966bae9b4bdf61088f4a20bd7e5f8aba54f12269fb5a6327e0672b28ae0` |
| `data.cpk::skill/0B6.pac::0B6.BED` | BED400 | 2240 | `f90a0966bae9b4bdf61088f4a20bd7e5f8aba54f12269fb5a6327e0672b28ae0` |
| `data.cpk::skill/0B7.pac::0B7.BED` | BED400 | 2240 | `f90a0966bae9b4bdf61088f4a20bd7e5f8aba54f12269fb5a6327e0672b28ae0` |
| `data.cpk::skill/0B8.pac::0B8.BED` | BED400 | 2240 | `f90a0966bae9b4bdf61088f4a20bd7e5f8aba54f12269fb5a6327e0672b28ae0` |
| `data.cpk::skill/0B9.pac::0B9.BED` | BED400 | 2240 | `f90a0966bae9b4bdf61088f4a20bd7e5f8aba54f12269fb5a6327e0672b28ae0` |
| `data.cpk::skill/0BA.pac::0BA.BED` | BED400 | 2240 | `f90a0966bae9b4bdf61088f4a20bd7e5f8aba54f12269fb5a6327e0672b28ae0` |
| `data.cpk::skill/0BB.pac::0BB.BED` | BED400 | 2240 | `f90a0966bae9b4bdf61088f4a20bd7e5f8aba54f12269fb5a6327e0672b28ae0` |
| `data.cpk::skill/0BC.pac::0BC.BED` | BED400 | 2240 | `f90a0966bae9b4bdf61088f4a20bd7e5f8aba54f12269fb5a6327e0672b28ae0` |
| `data.cpk::skill/0BD.pac::0BD.BED` | BED400 | 2240 | `f90a0966bae9b4bdf61088f4a20bd7e5f8aba54f12269fb5a6327e0672b28ae0` |
| `data.cpk::skill/0BE.pac::0BE.BED` | BED400 | 2240 | `f90a0966bae9b4bdf61088f4a20bd7e5f8aba54f12269fb5a6327e0672b28ae0` |
| `data.cpk::skill/0BF.pac::0BF.BED` | BED400 | 2240 | `f90a0966bae9b4bdf61088f4a20bd7e5f8aba54f12269fb5a6327e0672b28ae0` |
| `data.cpk::skill/0C0.pac::0C0.BED` | BED400 | 43104 | `8a9fd7837e57bb90ab276e9fc2ea3d03a0df9ba6d0a8902d98291e89c8b49991` |
| `data.cpk::skill/0C1.pac::0C1.BED` | BED400 | 78848 | `739040f11af537dda61757f1179ad1cf500d362846181d5f21abb5217dc77c7c` |
| `data.cpk::skill/0C2.pac::0C2.BED` | BED400 | 79968 | `824cda2410fe945bf4908b1870ddc3b8a27d364fb4db2acaa98ea6ed9e249e84` |
| `data.cpk::skill/0C3.pac::0C3.BED` | BED400 | 43104 | `d7ceb26121b05ea583f505284da4e7a29541cb867b487e0ca7544ffad8bd283c` |
| `data.cpk::skill/0C4.pac::0C4.BED` | BED400 | 78080 | `b2600372795e3a01946cac121ac74bcfb21d0faf1d5f2abb4747e34eed6de454` |
| `data.cpk::skill/0C5.pac::0C5.BED` | BED400 | 78880 | `ab1190046299d3f5e78c6ce9a0fcb83b6fa4a14fa753d14fc4ea7a66f7b80421` |
| `data.cpk::skill/0C6.pac::0C6.BED` | BED400 | 31936 | `289b22232e740d8ba72abb1e851f485aed2087258d80430483f48f18e3c1d471` |
| `data.cpk::skill/0C7.pac::0C7.BED` | BED400 | 26016 | `3701d912e6ae2c2a136113a89fca4bcb09ac2845070f0a3545336f5724ac3dfa` |
| `data.cpk::skill/0C8.pac::0C8.BED` | BED400 | 26016 | `292cb0af9d09e33a6e83a2bd7e998a2953e313e39dbaa04ed5f2af07937773ae` |
| `data.cpk::skill/0C9.pac::0C9.BED` | BED400 | 25664 | `437966b7f73b838ecea483ba1fc608225f6afd4d4c43037f5acd19bb0293b5a7` |
| `data.cpk::skill/0CA.pac::0CA.BED` | BED400 | 16768 | `988f9a9ea532cd50826575710ae23948bd20f6465d1941d6d31194821c0f4649` |
| `data.cpk::skill/0CB.pac::0CB.BED` | BED400 | 16768 | `a295dfdc135d1ca376cec14a40e63e581305ef5f8097efa45ad9163f467578a1` |
| `data.cpk::skill/0CC.pac::0CC.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/0CD.pac::0CD.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/0CE.pac::0CE.BED` | BED400 | 16768 | `ee2ad7847e91fb24e8a5c9908dc46791abfa51f2ac24f3251f7a5b7922bffe40` |
| `data.cpk::skill/0CF.pac::0CF.BED` | BED400 | 52384 | `30560e4665fa231fae46119c451cba907887add0b9b207f64c92bf36c146c7c5` |
| `data.cpk::skill/0D0.pac::0D0.BED` | BED400 | 26624 | `538d0ea6209f009749e72d5734d7994d22a1af15cd6d0832cac56125ab87ca39` |
| `data.cpk::skill/0D1.pac::0D1.BED` | BED400 | 47904 | `1f8b8d36d860ca72ba98df9d417ecb60cdabc25fc83f61b4844fe8f9b5621ba0` |
| `data.cpk::skill/0D2.pac::0D2.BED` | BED400 | 58464 | `39d898d189251988d6a7821c3b2f77b5497c7a2a1c671c8d0418de8af96511a1` |
| `data.cpk::skill/0D3.pac::0D3.BED` | BED400 | 127328 | `f4e5f620124d37ed26695279f60ca674e7a2832e650f74518be4b647120f372a` |
| `data.cpk::skill/0D4.pac::0D4.BED` | BED400 | 44224 | `57887f58bbd0d09756b7440669fa779c0e5cc0ebcb57cc1ec3c4d3411ee1ba1f` |
| `data.cpk::skill/0D5.pac::0D5.BED` | BED400 | 127328 | `a12407e983735b4b1b2103fa5c0b60d73d37a70e75cb6df8a3e447ee75e081d7` |
| `data.cpk::skill/0D6.pac::0D6.BED` | BED400 | 44224 | `e7f44bb2ad63c3c57bbf164ac95738748e643ad261180a8f89fef3d6575ab5ba` |
| `data.cpk::skill/0D7.pac::0D7.BED` | BED400 | 127520 | `bd5eb346bc49fc148a4884b8d4bafa90a3ee4e19952f38c9dc9b6cbe96362add` |
| `data.cpk::skill/0D8.pac::0D8.BED` | BED400 | 44416 | `ce98579c861ededcac6c8a368e286b7e8878f996b731abde9a6697a4f210ec9e` |
| `data.cpk::skill/0D9.pac::0D9.BED` | BED400 | 150112 | `c22913392e088f00df4821781463c8fba996f855984b4145950fc56dc03ba6ea` |
| `data.cpk::skill/0DA.pac::0DA.BED` | BED400 | 82784 | `b7bc80f5dedf6c979e9df9f1e01f4cb2eed1181893187038f068b11c3ea6e333` |
| `data.cpk::skill/0DB.pac::0DB.BED` | BED400 | 45344 | `ad21977ff3d7e3930502efa6f77204b2306174f2db98b46e48ae35d8e736f3e8` |
| `data.cpk::skill/0DC.pac::0DC.BED` | BED400 | 82784 | `bd7bd013be48d1c99a387654ec59bc0824a95b7d7b59deb280d57434a7836218` |
| `data.cpk::skill/0DD.pac::0DD.BED` | BED400 | 45760 | `64a4f4b3757c9668d1562f9ae53b2e4e08151244663ea6a61dd25718c3c6e810` |
| `data.cpk::skill/0DE.pac::0DE.BED` | BED400 | 83264 | `ece82d6d7cce22a4c65a4cba2d22ec769c700eccdcf6b787b184320ae3395f4d` |
| `data.cpk::skill/0DF.pac::0DF.BED` | BED400 | 45856 | `a3c3be12f12fa3a240a614b5734755889142c31eb251e557d58e9901586935aa` |
| `data.cpk::skill/0E0.pac::0E0.BED` | BED400 | 150112 | `6ca725500ce6984c079239af413e005e9f3d8efda9cdf9fe1367377295688deb` |
| `data.cpk::skill/0E1.pac::0E1.BED` | BED400 | 66592 | `7ab22329adda13c1e085e3cd6f88a816aff0e125d3c3450c54127420983cdd42` |
| `data.cpk::skill/0E2.pac::0E2.BED` | BED400 | 107072 | `e90bd44b10a380371426a31d382c026aabcfeb6bcdc99ce25145752d6b13b29f` |
| `data.cpk::skill/0E3.pac::0E3.BED` | BED400 | 44256 | `3de132f449f454b2ba34332b3bf97ffbffaa039510623959d55dccea07649db4` |
| `data.cpk::skill/0E4.pac::0E4.BED` | BED400 | 44256 | `74689a5a0c5f712e0d6d7a010f25c101758dae720136307d469156b89d0e0008` |
| `data.cpk::skill/0E5.pac::0E5.BED` | BED400 | 33120 | `10121e63b88a19ed6eb56d43bf9b7339c42025269b374d83c9a1a541b047b7aa` |
| `data.cpk::skill/0E6.pac::0E6.BED` | BED400 | 33120 | `66428542983883a6bf6864733c88106088bed9f48daf763bf5dc93150e5d9eed` |
| `data.cpk::skill/0E7.pac::0E7.BED` | BED400 | 119232 | `3a6fbc3416ff56572d6896aaf87de503d090cf924afef471f44dc5bd3752f1d5` |
| `data.cpk::skill/0E8.pac::0E8.BED` | BED400 | 119264 | `42ba5bfb4787bdc19764874ce39ef29f8b8bbacd05b76dbf041d0a42ade528d3` |
| `data.cpk::skill/0E9.pac::0E9.BED` | BED400 | 18816 | `eec275c04ae0ddea7e0d1eb4cfee8a4560800c3a89ce41f6db6075bd3a54a0c4` |
| `data.cpk::skill/0EA.pac::0EA.BED` | BED400 | 40576 | `8595a0882b00dce1eb4b018c8ab5953f7384b1e8f0fb605ff9c565734a10ae20` |
| `data.cpk::skill/0EB.pac::0EB.BED` | BED400 | 199168 | `dc52171a38daeb98013d99585af30cfaaf1d3ffad70f0c6e6c41a859c8df176a` |
| `data.cpk::skill/0EC.pac::0EC.BED` | BED400 | 84320 | `28f99ce0d9ccae71f83d2c6ea6040a9c0cbfa39a0e7dd7dcd6dcc27a5e19b208` |
| `data.cpk::skill/0ED.pac::0ED.BED` | BED400 | 86464 | `26bb572f51a0ed45aa7c74456c581f6f58e7a528e0c9b56e567620be05836c02` |
| `data.cpk::skill/0EE.pac::0EE.BED` | BED400 | 98528 | `d6c8eed803f8a385381dc25dc19b5d0f6e9892e95badc401ed707fe6b75c180e` |
| `data.cpk::skill/0EF.pac::0EF.BED` | BED400 | 149504 | `fd3ceb54ea544897067085a718363c9ece11ab3bf9aecbfd1d59a5e1ef9342d4` |
| `data.cpk::skill/0F0.pac::0F0.BED` | BED400 | 60064 | `a5823ec62e4b85226a87a0f5f9c74fbf94afe5af09c622b82be626f8a1bab16d` |
| `data.cpk::skill/0F1.pac::0F1.BED` | BED400 | 87744 | `370bce2f3d909638df02ec4800faee5ebda77c5403f50847175d5b45a5283896` |
| `data.cpk::skill/0F2.pac::0F2.BED` | BED400 | 87520 | `3837adcd3ab4e50d53400edd32cbf49b9dc38cf64ac9cffa24a6f37ce4a0ff72` |
| `data.cpk::skill/0F3.pac::0F3.BED` | BED400 | 70592 | `26f9734531c3dd19f555a11ccc2c8534c5a76fb866232200bb95eba626e9badf` |
| `data.cpk::skill/0F4.pac::0F4.BED` | BED400 | 61344 | `02bc01a2f572ae7c7c76a2ed8cee6e88c2173664e5da5fc7d8b9fc050299bb46` |
| `data.cpk::skill/0F5.pac::0F5.BED` | BED400 | 39296 | `87214e4955c36f3fefc47b2cea07da35d1f641980b7e7d5d127e1ecfe1764371` |
| `data.cpk::skill/0F6.pac::0F6.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/0F7.pac::0F7.BED` | BED400 | 157056 | `e30eab6e3e9615764731eb7b123a2bec199bd5815bcf3d3d553a3cdabc8d834e` |
| `data.cpk::skill/0F8.pac::0F8.BED` | BED400 | 199488 | `5f52ef6ac72f1b95d15a4fbe83b6dabfe32b69030a165bc2d43591dcdca36d8c` |
| `data.cpk::skill/0F9.pac::0F9.BED` | BED400 | 32352 | `2293f20ed5db7d33008bd7cef8a8b286e71447ba2e119ff6995878388ceba319` |
| `data.cpk::skill/0FA.pac::0FA.BED` | BED400 | 96192 | `cc82c74631babc1479ab17b31a88822066ad1720ab0dd17a269c38bea3aa8a8a` |
| `data.cpk::skill/0FB.pac::0FB.BED` | BED400 | 119168 | `1d91377fe45aeef453895ba5dddbba1478f5be382f6fd337998cd23c582416f4` |
| `data.cpk::skill/0FC.pac::0FC.BED` | BED400 | 73184 | `db6817799a44b5e029503bfc969df73be98829d0f00aa3535f087c533628fd70` |
| `data.cpk::skill/0FD.pac::0FD.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/0FE.pac::0FE.BED` | BED400 | 78880 | `cccc94c8fa654686feadaee1644649fe1b8c79f31ffc41ff93e3f3f6309cbc89` |
| `data.cpk::skill/0FF.pac::0FF.BED` | BED400 | 2240 | `a45bb16504f28fdf525e505b778343dd598fa810f5c4e6ff225c9567c78eca74` |
| `data.cpk::skill/100.pac::100.BED` | BED400 | 197152 | `7a20e5985d9a1bbc836c492c0b1aa60a197bd7cf5a4564681e3d842dde7abd58` |
| `data.cpk::skill/101.pac::101.BED` | BED400 | 207296 | `91a127a279d08128ad063c874bff5feba4e27417d5956d0efc8fb2726ec83acb` |
| `data.cpk::skill/102.pac::102.BED` | BED400 | 212416 | `9afc2cfe4bace35c33a8c20889773d8c38943a4ee93317367ccf9d23af42a987` |
| `data.cpk::skill/103.pac::103.BED` | BED400 | 80096 | `0c546bf19287e842a4c67e865eaa302803d9d418b4a0378dd6e448efe62d18cf` |
| `data.cpk::skill/104.pac::104.BED` | BED400 | 110848 | `b68b80091224d620afb3112482aee96b4018f631c4753615f43d652d36959ac9` |
| `data.cpk::skill/105.pac::105.BED` | BED400 | 61856 | `aeb522582b553a968cf8b5a59c29e051ecf6a37c37ff5494696652e05f02237e` |
| `data.cpk::skill/106.pac::106.BED` | BED400 | 39552 | `d37b1b96b337404c942fba493e11da2b33e4b0a0035786586525015d526c8b9c` |
| `data.cpk::skill/107.pac::107.BED` | BED400 | 86016 | `49c3c6c37b5a6aeef475d8d75f14afc9e89e93c1cef123056a0e924b6f09c6f1` |
| `data.cpk::skill/108.pac::108.BED` | BED400 | 106560 | `8c2096eef35e291b1d4522bd58c77722c5d271558d56ee09f76da788e17dfd4d` |
| `data.cpk::skill/109.pac::109.BED` | BED400 | 23328 | `1f54217507f75866a26c35bbacc4fceea39e58c130d5e83bb283146b4697e253` |
| `data.cpk::skill/10A.pac::10A.BED` | BED400 | 30304 | `5faeb78cf80acab9d1296d843fbb6e1705e3cb321c42d60b5816be6a37b309ac` |
| `data.cpk::skill/10B.pac::10B.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/10C.pac::10C.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/10D.pac::10D.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/10E.pac::10E.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/10F.pac::10F.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/110.pac::110.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/111.pac::111.BED` | BED400 | 2720 | `96e4cba4b83fc00732687e18dbae518d0a12ca17ec2d31dcc842ee88c62dfc6e` |
| `data.cpk::skill/112.pac::112.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/113.pac::113.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/114.pac::114.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/115.pac::115.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/116.pac::116.BED` | BED400 | 126304 | `4cf1b65a5df516f11b38328cde9ce905184ba91226bcf483bd90ea12bcb1274f` |
| `data.cpk::skill/117.pac::117.BED` | BED400 | 92256 | `131c0ad08318753f9603911c6e94c3a2a6d923abe8573ce0e3f923356981983b` |
| `data.cpk::skill/118.pac::118.BED` | BED400 | 56096 | `1bd75a7304d925f92f08d72c833dcb542d385b73123680fd1683e277dfd21464` |
| `data.cpk::skill/119.pac::119.BED` | BED400 | 8416 | `763eb9422f1d8651afe09f8e9dc7cf8b202efcb7f921b5f288b282ba91359971` |
| `data.cpk::skill/11A.pac::11A.BED` | BED400 | 114848 | `11a5ef217668ce818e08a2f710bdf46958368c9fadc4f48411f7c60e4fb20582` |
| `data.cpk::skill/11B.pac::11B.BED` | BED400 | 32416 | `2d04d91d1387588f8ca63c8583b5dbc94686b5b055192359cd1b831ee9b3088b` |
| `data.cpk::skill/11C.pac::11C.BED` | BED400 | 430656 | `038b20b5d2944fbdc4c47409567ef659c6dd0cd6e770bf2b308d03270863424c` |
| `data.cpk::skill/11D.pac::11D.BED` | BED400 | 159040 | `59fdcb728f9903fc17e36bca7e27b2b21265b35052fbec09f5a7301c6c497687` |
| `data.cpk::skill/11E.pac::11E.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/11F.pac::11F.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/120.pac::120.BED` | BED400 | 43104 | `8a9fd7837e57bb90ab276e9fc2ea3d03a0df9ba6d0a8902d98291e89c8b49991` |
| `data.cpk::skill/121.pac::121.BED` | BED400 | 43104 | `8a9fd7837e57bb90ab276e9fc2ea3d03a0df9ba6d0a8902d98291e89c8b49991` |
| `data.cpk::skill/122.pac::122.BED` | BED400 | 78848 | `739040f11af537dda61757f1179ad1cf500d362846181d5f21abb5217dc77c7c` |
| `data.cpk::skill/123.pac::123.BED` | BED400 | 78848 | `739040f11af537dda61757f1179ad1cf500d362846181d5f21abb5217dc77c7c` |
| `data.cpk::skill/124.pac::124.BED` | BED400 | 78848 | `739040f11af537dda61757f1179ad1cf500d362846181d5f21abb5217dc77c7c` |
| `data.cpk::skill/125.pac::125.BED` | BED400 | 79968 | `824cda2410fe945bf4908b1870ddc3b8a27d364fb4db2acaa98ea6ed9e249e84` |
| `data.cpk::skill/126.pac::126.BED` | BED400 | 43104 | `d7ceb26121b05ea583f505284da4e7a29541cb867b487e0ca7544ffad8bd283c` |
| `data.cpk::skill/127.pac::127.BED` | BED400 | 78080 | `b2600372795e3a01946cac121ac74bcfb21d0faf1d5f2abb4747e34eed6de454` |
| `data.cpk::skill/128.pac::128.BED` | BED400 | 78080 | `b2600372795e3a01946cac121ac74bcfb21d0faf1d5f2abb4747e34eed6de454` |
| `data.cpk::skill/129.pac::129.BED` | BED400 | 78880 | `ab1190046299d3f5e78c6ce9a0fcb83b6fa4a14fa753d14fc4ea7a66f7b80421` |
| `data.cpk::skill/12A.pac::12A.BED` | BED400 | 43104 | `8a9fd7837e57bb90ab276e9fc2ea3d03a0df9ba6d0a8902d98291e89c8b49991` |
| `data.cpk::skill/12B.pac::12B.BED` | BED400 | 43104 | `8a9fd7837e57bb90ab276e9fc2ea3d03a0df9ba6d0a8902d98291e89c8b49991` |
| `data.cpk::skill/12C.pac::12C.BED` | BED400 | 78848 | `739040f11af537dda61757f1179ad1cf500d362846181d5f21abb5217dc77c7c` |
| `data.cpk::skill/12D.pac::12D.BED` | BED400 | 79968 | `824cda2410fe945bf4908b1870ddc3b8a27d364fb4db2acaa98ea6ed9e249e84` |
| `data.cpk::skill/12E.pac::12E.BED` | BED400 | 47904 | `1f8b8d36d860ca72ba98df9d417ecb60cdabc25fc83f61b4844fe8f9b5621ba0` |
| `data.cpk::skill/12F.pac::12F.BED` | BED400 | 58464 | `39d898d189251988d6a7821c3b2f77b5497c7a2a1c671c8d0418de8af96511a1` |
| `data.cpk::skill/130.pac::130.BED` | BED400 | 16768 | `ee2ad7847e91fb24e8a5c9908dc46791abfa51f2ac24f3251f7a5b7922bffe40` |
| `data.cpk::skill/131.pac::131.BED` | BED400 | 16768 | `988f9a9ea532cd50826575710ae23948bd20f6465d1941d6d31194821c0f4649` |
| `data.cpk::skill/132.pac::132.BED` | BED400 | 16768 | `a295dfdc135d1ca376cec14a40e63e581305ef5f8097efa45ad9163f467578a1` |
| `data.cpk::skill/133.pac::133.BED` | BED400 | 26016 | `3701d912e6ae2c2a136113a89fca4bcb09ac2845070f0a3545336f5724ac3dfa` |
| `data.cpk::skill/134.pac::134.BED` | BED400 | 25664 | `437966b7f73b838ecea483ba1fc608225f6afd4d4c43037f5acd19bb0293b5a7` |
| `data.cpk::skill/135.pac::135.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/136.pac::136.BED` | BED400 | 61344 | `02bc01a2f572ae7c7c76a2ed8cee6e88c2173664e5da5fc7d8b9fc050299bb46` |
| `data.cpk::skill/137.pac::137.BED` | BED400 | 78880 | `ab1190046299d3f5e78c6ce9a0fcb83b6fa4a14fa753d14fc4ea7a66f7b80421` |
| `data.cpk::skill/138.pac::138.BED` | BED400 | 26624 | `538d0ea6209f009749e72d5734d7994d22a1af15cd6d0832cac56125ab87ca39` |
| `data.cpk::skill/139.pac::139.BED` | BED400 | 78880 | `ab1190046299d3f5e78c6ce9a0fcb83b6fa4a14fa753d14fc4ea7a66f7b80421` |
| `data.cpk::skill/13A.pac::13A.BED` | BED400 | 78848 | `739040f11af537dda61757f1179ad1cf500d362846181d5f21abb5217dc77c7c` |
| `data.cpk::skill/13B.pac::13B.BED` | BED400 | 78848 | `739040f11af537dda61757f1179ad1cf500d362846181d5f21abb5217dc77c7c` |
| `data.cpk::skill/13C.pac::13C.BED` | BED400 | 78848 | `739040f11af537dda61757f1179ad1cf500d362846181d5f21abb5217dc77c7c` |
| `data.cpk::skill/13D.pac::13D.BED` | BED400 | 39328 | `8ac1679bb4f9ea38dd86766a3bfe38ac15004d7339a82e18f4a89a78e5bd76d5` |
| `data.cpk::skill/13E.pac::13E.BED` | BED400 | 33120 | `66428542983883a6bf6864733c88106088bed9f48daf763bf5dc93150e5d9eed` |
| `data.cpk::skill/13F.pac::13F.BED` | BED400 | 33120 | `10121e63b88a19ed6eb56d43bf9b7339c42025269b374d83c9a1a541b047b7aa` |
| `data.cpk::skill/140.pac::140.BED` | BED400 | 45344 | `ad21977ff3d7e3930502efa6f77204b2306174f2db98b46e48ae35d8e736f3e8` |
| `data.cpk::skill/141.pac::141.BED` | BED400 | 45760 | `64a4f4b3757c9668d1562f9ae53b2e4e08151244663ea6a61dd25718c3c6e810` |
| `data.cpk::skill/142.pac::142.BED` | BED400 | 45856 | `a3c3be12f12fa3a240a614b5734755889142c31eb251e557d58e9901586935aa` |
| `data.cpk::skill/143.pac::143.BED` | BED400 | 151456 | `9baf5fb80a15aca34bd16d162f498ab8418b207973ba6381f6f1c4ea06e423ca` |
| `data.cpk::skill/144.pac::144.BED` | BED400 | 151424 | `6568edd9a9630ce9e047d17a582025ca99bb79a913403755c4f1a7fc563e8c61` |
| `data.cpk::skill/145.pac::145.BED` | BED400 | 38240 | `4d2354d7a8c0ef08ad2cb80330bd755e89025bac58320d4996c1ce22181ea1ca` |
| `data.cpk::skill/146.pac::146.BED` | BED400 | 87584 | `be354c81285c25d3b363100b672b80b31fc4cc6ec83cbdf84d71ad4d8e9e9518` |
| `data.cpk::skill/147.pac::147.BED` | BED400 | 57152 | `7ab5700e472737e0ced96569a30eac1e7d123419a4158b893a4a78a5bbe8e696` |
| `data.cpk::skill/148.pac::148.BED` | BED400 | 160640 | `797e10c5c3dddd1a7e505670b5d34a6ac4608fd52092f8dbb046e0ca2b72ee95` |
| `data.cpk::skill/149.pac::149.BED` | BED400 | 37184 | `3b1789fa8d9b77b8b68d887c3c2da00b989c5f31697dc8b3a5392301c0f3bc49` |
| `data.cpk::skill/14A.pac::14A.BED` | BED400 | 29632 | `35fcd4b3375fcd50e51d69b846a6ffd9831005394f2f47f7d2bcb38ab88ccbc5` |
| `data.cpk::skill/14B.pac::14B.BED` | BED400 | 33856 | `bdd2e3621e7f311ebe987eb4bde9df4cecd907e361a27f6774f4cbc528bb3767` |
| `data.cpk::skill/14C.pac::14C.BED` | BED400 | 15104 | `7aa8074e0640fbdd3b767e021dd5507ee9f9b6bc0f0c470aaa0fc65adff3dfd2` |
| `data.cpk::skill/14D.pac::14D.BED` | BED400 | 9728 | `a984841402522806c228c6c9e86365a45e33a97607b6afabbc3362383b260c6d` |
| `data.cpk::skill/14E.pac::14E.BED` | BED400 | 178016 | `2082f625fdc6271ec28bef027bbcb6bcededd57ef4cf882c336a611268db360c` |
| `data.cpk::skill/14F.pac::14F.BED` | BED400 | 98112 | `5f567ed9fae13772ea5bc7d3896fbc5b5eab3c9f0f2ed48fe33c4a9e2bfa7e33` |
| `data.cpk::skill/150.pac::150.BED` | BED400 | 9024 | `4e18eebcab8fd6bd52211849855c5e7be8a28695244dfecb713905a5bab8929f` |
| `data.cpk::skill/151.pac::151.BED` | BED400 | 184896 | `d33379d8d2298e5d67404a40448bf116f3e25690a80a6a3cd6e5e9a0ddbe32f5` |
| `data.cpk::skill/152.pac::152.BED` | BED400 | 32928 | `e09fd7eaf079f2e82468d405d8b062863364d674f1945f18865b93d5eb51b0a7` |
| `data.cpk::skill/153.pac::153.BED` | BED400 | 178016 | `cc228d7621d83c0279f19a917d36d844b73f04284692852a155107ef14016e20` |
| `data.cpk::skill/154.pac::154.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/155.pac::155.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/156.pac::156.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/157.pac::157.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/158.pac::158.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/159.pac::159.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/15A.pac::15A.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/15B.pac::15B.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/15C.pac::15C.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/15D.pac::15D.BED` | BED400 | 252768 | `bae0f50959f41d4da2387981fca0014f5711bbd2c11a2e836e2d5d42a85b44bb` |
| `data.cpk::skill/15E.pac::15E.BED` | BED400 | 409920 | `2c70daae55e1797834fb9d8938a5caf09bafdfeb233cdf420b6182da753f5654` |
| `data.cpk::skill/15F.pac::15F.BED` | BED400 | 161280 | `981b68960402938cc8d93daed15c4b376331f1fe7736ad4f428ec50dc9b3f448` |
| `data.cpk::skill/160.pac::160.BED` | BED400 | 95808 | `d05670294f88cb4f19cc940c7c53abe409bc8bcf43057b21bf595f602d7eeb2d` |
| `data.cpk::skill/161.pac::161.BED` | BED400 | 12384 | `30acb47fa8ee272970c0806df9eb416fb572dffd4078d5f67ef7c461498896b7` |
| `data.cpk::skill/162.pac::162.BED` | BED400 | 16928 | `d4d4179cd8d7c4756bd52a11f7a497bd2f68a3b8b4cecd92db42aee403e2e5b9` |
| `data.cpk::skill/163.pac::163.BED` | BED400 | 23040 | `8c5f5180e2f567604cccf9bd46613a51172579220eef98da49e1d295d9f3b712` |
| `data.cpk::skill/164.pac::164.BED` | BED400 | 18432 | `a0fa5bcaab5e6eb8169be6f75d26cf5914e93ae9f44177a6479a2647b1cd72b8` |
| `data.cpk::skill/165.pac::165.BED` | BED400 | 18432 | `d36da074d5fca3e5a4ec5f1e0bbf6c0b6dad3f14b298093c51a4a0452512aaa8` |
| `data.cpk::skill/166.pac::166.BED` | BED400 | 215616 | `fc71909b34e3a7faf29a5f5981d298290b9703aed99be801544450f33de0a873` |
| `data.cpk::skill/167.pac::167.BED` | BED400 | 215616 | `b88626aedda9f41debb84385630b86ddf796253da8d340390bce5870c4214f43` |
| `data.cpk::skill/168.pac::168.BED` | BED400 | 154848 | `c3fc7580f60434b9b82da846336affe5ec018536b2555b5e89fa84d234d725e2` |
| `data.cpk::skill/169.pac::169.BED` | BED400 | 123424 | `3da7cbbdc4f45bae6514c7b53f0602cf0932bed5c4367c53eb4e69b2cddb7811` |
| `data.cpk::skill/16A.pac::16A.BED` | BED400 | 25760 | `fb2020fdcd8e90f5df3e986b66ade6e764e7c99022fc6a614349ab75c455826f` |
| `data.cpk::skill/16B.pac::16B.BED` | BED400 | 10752 | `1529d5f2831a1a34651321eac1df4cb67fc14206749fc37c0e16a4f03671061f` |
| `data.cpk::skill/16C.pac::16C.BED` | BED400 | 171808 | `193c27b749eb48ffebb01a550258f65b2426384a34b46bbbaf55a04047ac9a08` |
| `data.cpk::skill/16D.pac::16D.BED` | BED400 | 46144 | `3ff50fafbd27262cca15a27c93a5fd78a6f727ae6a322f9e5f82bbb143abfb32` |
| `data.cpk::skill/16E.pac::16E.BED` | BED400 | 79616 | `ecf907d6378121637f8e62f5d676d00c262cf1363e11e683f26ff461b6d650a5` |
| `data.cpk::skill/16F.pac::16F.BED` | BED400 | 168640 | `0e765a80b1b077ade79884f4ed137f78df1de9d57cb19e43d825e0c3a82402d0` |
| `data.cpk::skill/170.pac::170.BED` | BED400 | 167936 | `c58a57262e48be0d722c965326e8902ae90d638e320ef60074f9d2295dbd5298` |
| `data.cpk::skill/171.pac::171.BED` | BED400 | 191232 | `b117ee0f7430110b02b57c6605d8a75ea21b96fce2448e7a9a05dfb7732c7dac` |
| `data.cpk::skill/172.pac::172.BED` | BED400 | 109824 | `daefc6a496e3ce5adc0df32d21942461e39b97160aa47482a540054b347e258a` |
| `data.cpk::skill/173.pac::173.BED` | BED400 | 163968 | `96bcf57f955c821044fc86287dcdc91a0e73cdd55ad300f64a18706a69f8a018` |
| `data.cpk::skill/174.pac::174.BED` | BED400 | 248736 | `a3adfb497d1e18ed8e558d3c39c15699682b10a87495ef506c1a6227602c056d` |
| `data.cpk::skill/175.pac::175.BED` | BED400 | 248736 | `2e2ae6e55f50b2e9d97d02e8641939899de92966b22b0e8aea248b595546f5eb` |
| `data.cpk::skill/176.pac::176.BED` | BED400 | 248736 | `a622b031321ebf76655a0d8c99057f5a02c87ee162aefe2d76409908662408b6` |
| `data.cpk::skill/177.pac::177.BED` | BED400 | 248736 | `a6b354f6f5af3e9a8775325f44f925612cc544f476e7d09cb325dc251cbe7dcc` |
| `data.cpk::skill/178.pac::178.BED` | BED400 | 64608 | `b531c6d7c8cc9821d563c336d7676af4fb3edd1c186869b4974c4e8a8e817151` |
| `data.cpk::skill/179.pac::179.BED` | BED400 | 47264 | `f1354e9c8de6b3b0ed90b1087293b71a4665a8d4a0bf84a8c2956c537be2aebe` |
| `data.cpk::skill/17A.pac::17A.BED` | BED400 | 155136 | `b37cce288f485d3229e7cb414e82346592124ae02f5a1ad503e5244cf88469d0` |
| `data.cpk::skill/17B.pac::17B.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/17C.pac::17C.BED` | BED400 | 98816 | `e273086eb62fe3ab998c690e67fe04ef3dc9987f3ec9a58400fd659d986878d3` |
| `data.cpk::skill/17D.pac::17D.BED` | BED400 | 49728 | `37a47c0d3e83b1561d11495c0950637fc9f37b2e7e14d103ac14a12fadfe8312` |
| `data.cpk::skill/17E.pac::17E.BED` | BED400 | 24672 | `fb1c4673efa3c1ebe6d62e2c611136cf585fdad58ae1d8f44aaefd808d6adfc2` |
| `data.cpk::skill/17F.pac::17F.BED` | BED400 | 10784 | `84ad154a6ed023d31b1b5014ab84be35b666aa4deda2898dfb7b89906b8f61dd` |
| `data.cpk::skill/180.pac::180.BED` | BED400 | 11872 | `e583ff014155d3509606875663cc233f72db16e52067e9f458c1447818d23159` |
| `data.cpk::skill/181.pac::181.BED` | BED400 | 346560 | `adfd79ecc8027b21578d0d4cadfe732e7fcb7fd005adf3af372e0fe44828b8e9` |
| `data.cpk::skill/182.pac::182.BED` | BED400 | 199296 | `0340d83c3168f5d04fdf373f84f492f350ebcb06a1322dd3b238c857f94c2cc2` |
| `data.cpk::skill/183.pac::183.BED` | BED400 | 40768 | `7f125e10e6c3800dced96ebb953f316f3b2b48edbd271695be1d517357811588` |
| `data.cpk::skill/184.pac::184.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/185.pac::185.BED` | BED400 | 101824 | `50098c2878b09ef2863219ca5f4e46724bd1792aa015966afb1485766da0a49c` |
| `data.cpk::skill/186.pac::186.BED` | BED400 | 18880 | `5cb60938c9742a840ea52432ed8059a6d789e5a46b5006b121f818145a93d279` |
| `data.cpk::skill/187.pac::187.BED` | BED400 | 68576 | `5a0818be8b0b1070b12ab915ed8d1c407aceb5ccd2eef606de23deccb755beb8` |
| `data.cpk::skill/188.pac::188.BED` | BED400 | 99520 | `b01c3fe33c079424e92f65ec7b762e83230055e51ff1c1083209a5eaab966ef1` |
| `data.cpk::skill/189.pac::189.BED` | BED400 | 99520 | `b01c3fe33c079424e92f65ec7b762e83230055e51ff1c1083209a5eaab966ef1` |
| `data.cpk::skill/18A.pac::18A.BED` | BED400 | 43104 | `461a346ea8eb595f74d921db754414a6f792f2a3afefc1c7b08873504c3bc17e` |
| `data.cpk::skill/18B.pac::18B.BED` | BED400 | 342720 | `9e1f195b924e195602103d2a3e6c35914f753626904f2957d79875d25835b6c1` |
| `data.cpk::skill/18C.pac::18C.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/18D.pac::18D.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/18E.pac::18E.BED` | BED400 | 212416 | `c807b1d647ffa9b4306e2d841736b178649b30bc93eeb954cdf4efde43c97a80` |
| `data.cpk::skill/18F.pac::18F.BED` | BED400 | 407104 | `ac2e2f78bb6b027aa917200cd403bc7da38874d273a6d9ded0357e42d4e9de54` |
| `data.cpk::skill/190.pac::190.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/191.pac::191.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/192.pac::192.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/193.pac::193.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/194.pac::194.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/195.pac::195.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/196.pac::196.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/197.pac::197.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/198.pac::198.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/199.pac::199.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/19A.pac::19A.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/19B.pac::19B.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/19C.pac::19C.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/19D.pac::19D.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/19E.pac::19E.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/19F.pac::19F.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1A0.pac::1A0.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1A1.pac::1A1.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1A2.pac::1A2.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1A3.pac::1A3.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1A4.pac::1A4.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1A5.pac::1A5.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1A6.pac::1A6.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1A7.pac::1A7.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1A8.pac::1A8.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1A9.pac::1A9.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1AA.pac::1AA.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1AB.pac::1AB.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1AC.pac::1AC.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1AD.pac::1AD.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1AE.pac::1AE.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1AF.pac::1AF.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1B0.pac::1B0.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1B1.pac::1B1.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1B2.pac::1B2.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1B3.pac::1B3.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1B4.pac::1B4.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1B5.pac::1B5.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1B6.pac::1B6.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1B7.pac::1B7.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1B8.pac::1B8.BED` | BED400 | 78848 | `739040f11af537dda61757f1179ad1cf500d362846181d5f21abb5217dc77c7c` |
| `data.cpk::skill/1B9.pac::1B9.BED` | BED400 | 47904 | `1f8b8d36d860ca72ba98df9d417ecb60cdabc25fc83f61b4844fe8f9b5621ba0` |
| `data.cpk::skill/1BA.pac::1BA.BED` | BED400 | 43104 | `8a9fd7837e57bb90ab276e9fc2ea3d03a0df9ba6d0a8902d98291e89c8b49991` |
| `data.cpk::skill/1BB.pac::1BB.BED` | BED400 | 43104 | `8a9fd7837e57bb90ab276e9fc2ea3d03a0df9ba6d0a8902d98291e89c8b49991` |
| `data.cpk::skill/1BC.pac::1BC.BED` | BED400 | 78848 | `739040f11af537dda61757f1179ad1cf500d362846181d5f21abb5217dc77c7c` |
| `data.cpk::skill/1BD.pac::1BD.BED` | BED400 | 38240 | `4d2354d7a8c0ef08ad2cb80330bd755e89025bac58320d4996c1ce22181ea1ca` |
| `data.cpk::skill/1BE.pac::1BE.BED` | BED400 | 57152 | `7ab5700e472737e0ced96569a30eac1e7d123419a4158b893a4a78a5bbe8e696` |
| `data.cpk::skill/1BF.pac::1BF.BED` | BED400 | 37184 | `3b1789fa8d9b77b8b68d887c3c2da00b989c5f31697dc8b3a5392301c0f3bc49` |
| `data.cpk::skill/1C0.pac::1C0.BED` | BED400 | 33856 | `bdd2e3621e7f311ebe987eb4bde9df4cecd907e361a27f6774f4cbc528bb3767` |
| `data.cpk::skill/1C1.pac::1C1.BED` | BED400 | 9024 | `4e18eebcab8fd6bd52211849855c5e7be8a28695244dfecb713905a5bab8929f` |
| `data.cpk::skill/1C2.pac::1C2.BED` | BED400 | 32928 | `e09fd7eaf079f2e82468d405d8b062863364d674f1945f18865b93d5eb51b0a7` |
| `data.cpk::skill/1C3.pac::1C3.BED` | BED400 | 184896 | `d33379d8d2298e5d67404a40448bf116f3e25690a80a6a3cd6e5e9a0ddbe32f5` |
| `data.cpk::skill/1C4.pac::1C4.BED` | BED400 | 178016 | `cc228d7621d83c0279f19a917d36d844b73f04284692852a155107ef14016e20` |
| `data.cpk::skill/1C5.pac::1C5.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1C6.pac::1C6.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1C7.pac::1C7.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1C8.pac::1C8.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1C9.pac::1C9.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1CA.pac::1CA.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1CB.pac::1CB.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1CC.pac::1CC.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1CD.pac::1CD.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1CE.pac::1CE.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1CF.pac::1CF.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1D0.pac::1D0.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1D1.pac::1D1.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1D2.pac::1D2.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1D3.pac::1D3.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1D4.pac::1D4.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1D5.pac::1D5.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1D6.pac::1D6.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1D7.pac::1D7.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1D8.pac::1D8.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1D9.pac::1D9.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1DA.pac::1DA.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1DB.pac::1DB.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1DC.pac::1DC.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1DD.pac::1DD.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1DE.pac::1DE.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1DF.pac::1DF.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1E0.pac::1E0.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1E1.pac::1E1.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1E2.pac::1E2.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1E3.pac::1E3.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1E4.pac::1E4.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1E5.pac::1E5.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1E6.pac::1E6.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1E7.pac::1E7.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1E8.pac::1E8.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1E9.pac::1E9.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1EA.pac::1EA.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1EB.pac::1EB.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1EC.pac::1EC.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1ED.pac::1ED.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1EE.pac::1EE.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1EF.pac::1EF.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1F0.pac::1F0.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1F1.pac::1F1.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1F2.pac::1F2.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1F3.pac::1F3.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1F4.pac::1F4.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1F5.pac::1F5.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1F6.pac::1F6.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1F7.pac::1F7.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1F8.pac::1F8.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1F9.pac::1F9.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1FA.pac::1FA.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1FB.pac::1FB.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1FC.pac::1FC.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1FD.pac::1FD.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::skill/1FE.pac::1FE.BED` | BED400 | 21440 | `9704593f762577ab2f82cb9088ec094c2c340d9c2e599d3812033835927b62b7` |
| `data.cpk::system/tool/effect/TEST.EPL` | EPL | 79440 | `c364e447f53aef21a41be7f1aa554d8dc9ce8a8dffa3e71f1bdcc683e965cafe` |
| `data.cpk::system/tool/effect/TV_EFF.EPL` | EPL | 14800 | `0a4136c277c67fefb3e8a10435c33f799d3ffedb4ff42b4441f46a4a7eb1f6db` |
| `data.cpk::system/tool/effect/ep/TEST.EP` | EP | 292 | `314c5aeaffc86b75477f5727905df6b8656af1f0849a72f85d7382d7cd40ce08` |
| `data.cpk::system/tool/effect/hlp/TEST.PB` | PB | 900 | `166b01f0d77998aa607a9be737a4a3eddf9dbbe0e74de0ae91dcde22aa608aa0` |
| `data.cpk::system/tool/effect/sample/B_CAMRA0.EP` | EP | 292 | `05d4b59e5cf8dfce56008546d64a99ed1660634bd98628ca3c43beb37d734302` |
| `data.cpk::system/tool/effect/sample/B_DELP0.EP` | EP | 356 | `25454b92325e501bcb434641fab0accb56ce4ea6d3aac9947a6eb794fe25e85c` |
| `data.cpk::system/tool/effect/sample/B_DIST0.EP` | EP | 292 | `193e7bc5f46cf51cc7faad1697bdefbf006f8a0b28c117653f8deac17be72d11` |
| `data.cpk::system/tool/effect/sample/B_FULL0.EP` | EP | 292 | `a57d4b986bcaa2af0ff3e719651281feebb851b86fbbb03b7677e6c78369415a` |
| `data.cpk::system/tool/effect/sample/B_MULTI0.EP` | EP | 292 | `8497f9035ffedb76c0ba476a90d03cec735b9ba1d5193dcda414c091441cd84e` |
| `data.cpk::system/tool/effect/sample/B_PARAL0.EP` | EP | 292 | `092371ac53ff44604f32b84b9540247df5858827bbcec462a90959dd16a70ccc` |
| `data.cpk::system/tool/effect/sample/B_RIPLE0.EP` | EP | 292 | `85ca0f0b64a21e67d10d746b3c8d7fe55839af5b8aa663f659ff560d509104a8` |
| `data.cpk::system/tool/effect/sample/C_NOMAL0.EP` | EP | 8112 | `eb31ce187a138b712517d3c040fd22ad26fd8bfe77e214e66a0bf340bf26fd90` |
| `data.cpk::system/tool/effect/sample/D_BUG0.EP` | EP | 228 | `c62adb121ac4ee95240db8ea31dddbd99d28987aac146b5e1e34a953c4545719` |
| `data.cpk::system/tool/effect/sample/D_BUG1.EP` | EP | 228 | `b6eaccde9dc700fc13ae1ed3b2465a27d843511d680564a13cb7f499a2c32ada` |
| `data.cpk::system/tool/effect/sample/D_P2_V20.EP` | EP | 196 | `9f1f5bfe5787cc16af3bcd3b7786919f0200e9754b2f2fad9eacccdf9aa4a423` |
| `data.cpk::system/tool/effect/sample/D_POINT0.EP` | EP | 196 | `50c22a8c6547e9ea738c3f8b1aeebc97970ad4f198130f374bb8921c3b4cf52c` |
| `data.cpk::system/tool/effect/sample/E_FLARE0.EP` | EP | 116 | `0479c55cd336a0922eaf1891235835daf707d98b061a0834e377166b1053fd3d` |
| `data.cpk::system/tool/effect/sample/E_QUAKE0.EP` | EP | 68 | `0203db6756b182f336095499db7cd88cdd5d08bebc66b4c0338b6b73b6f7c05f` |
| `data.cpk::system/tool/effect/sample/E_QUAKE20.EP` | EP | 68 | `7208dfa81db21c80bc6e6cd59daf2d7e300f24f6c50c65027f1c1761bbc07065` |
| `data.cpk::system/tool/effect/sample/E_RAIN0.EP` | EP | 116 | `3f6fc00ef97b84d04946820d02400a6b72220ec16ed30513dfae24c7c0f43a06` |
| `data.cpk::system/tool/effect/sample/F_FULL0.EP` | EP | 276 | `21a9fb97dffe671fe4aaa0bc54ec2ae8938ecf86998690cc6c1eccebfab6174b` |
| `data.cpk::system/tool/effect/sample/F_MULTI0.EP` | EP | 276 | `8396fb69eabe00f22db1dc0aba1cc532c9ccc0170726a538eec41ded6b16293c` |
| `data.cpk::system/tool/effect/sample/F_NOMAL0.EP` | EP | 100 | `b9fbdfc3c297d38e81bd5efc4eeaea2f4c03ef02562024a6b00224d34bb3d832` |
| `data.cpk::system/tool/effect/sample/G_ASPAK0.EP` | EP | 324 | `0e23226536de1b2224eb510a56f450288ea3176c0f62d0f81dfd0fb8f9f1dd87` |
| `data.cpk::system/tool/effect/sample/G_ATOM0.EP` | EP | 308 | `5e586568e3632ed37a6073f96bd9e4c758aae355fa8d03a82076724407c0d317` |
| `data.cpk::system/tool/effect/sample/G_CSPAK0.EP` | EP | 324 | `a67c63d5c3cfaf781d693bfd130f507ff7ee3a81d2c315bb53d329afcfb58736` |
| `data.cpk::system/tool/effect/sample/G_HSPAK0.EP` | EP | 308 | `12353eb5722f8f856536d7d42c06ecd008ac2f910fde81d971ce92cbfa1fc5e3` |
| `data.cpk::system/tool/effect/sample/G_RSPHR0.EP` | EP | 324 | `8e0692ec899dfbf45374941ce995bdc389c35d794df9ec14eedcad1dbe25778b` |
| `data.cpk::system/tool/effect/sample/G_SMOKE0.EP` | EP | 324 | `a25629a7384935dd6b469d4ae7629924bc544bd066ff2679576c30742692dd86` |
| `data.cpk::system/tool/effect/sample/G_SPARK0.EP` | EP | 308 | `829e4f9e3cbccbc87a46bd1850d2e1bfd8a2ec32c030ab27d01a06da27128308` |
| `data.cpk::system/tool/effect/sample/G_SPHRE0.EP` | EP | 308 | `9093416b2001482509a878c6e6fbcc6afd0dabe220646977ee29fcdcd00b2a8a` |
| `data.cpk::system/tool/effect/sample/G_SPIRL0.EP` | EP | 324 | `b2b654683ffd01a83636f82a669b5f168937675dfc4888c91b856c87e6db66d2` |
| `data.cpk::system/tool/effect/sample/I_ASPAK.EP` | EP | 292 | `5f6198fbaa13b5438a777f42fd7841c0b14ad829feacf45db2de731dd84bc9c2` |
| `data.cpk::system/tool/effect/sample/I_ASPAK0.EP` | EP | 17824 | `5ab421d21583470805c65482d2734828e64045aa4771f9cf466298e531811a6d` |
| `data.cpk::system/tool/effect/sample/I_ATOM.EP` | EP | 292 | `789299b5d72876ad302af98e43fe6ef55ca511642c00e21382efca28592989ce` |
| `data.cpk::system/tool/effect/sample/I_ATOM0.EP` | EP | 17824 | `3437261ee8a180f3a6e7cc03831853fdf8b7eb895a8c3188b8655188d960b68e` |
| `data.cpk::system/tool/effect/sample/I_CSPAK.EP` | EP | 308 | `ae9b55c7d795249587f8ddbcb069f6e5ca6e6c17d1b7630a5f55e68c316d1221` |
| `data.cpk::system/tool/effect/sample/I_CSPAK0.EP` | EP | 17840 | `be0afe31772649d301bcdfafabc374548687b324b0d7f9c6b598491ef487734c` |
| `data.cpk::system/tool/effect/sample/I_HSPAK.EP` | EP | 292 | `499ebc436ec2a0888eba311fee2c66cf6bffce6bda3c9e2f5c008ad76295ff61` |
| `data.cpk::system/tool/effect/sample/I_HSPAK0.EP` | EP | 17824 | `e2ffa0af9c7e64b13141a8d8c9ae18fdbc715f8e1e16815baaeb2bd0849d1a8d` |
| `data.cpk::system/tool/effect/sample/I_RSPHR.EP` | EP | 308 | `704dd2867a1eacd3384d73e42d4bf9c1ac442b6cf8c7e9b20e0471d95a79cb88` |
| `data.cpk::system/tool/effect/sample/I_RSPHR0.EP` | EP | 17840 | `d67f900c192e681f9ff12197d8ae461a96716a27e1a1d75f06eb56cee47c66c7` |
| `data.cpk::system/tool/effect/sample/I_SMOKE.EP` | EP | 292 | `e33d60e71bdf4d846cdea997d02ebbe90622e30584565c11e2bf0598dc8de938` |
| `data.cpk::system/tool/effect/sample/I_SMOKE0.EP` | EP | 17840 | `75daa9659e9d73902ae067a6e2d290b71888eef3acd5ceef1e031d5508d96bd7` |
| `data.cpk::system/tool/effect/sample/I_SPARK.EP` | EP | 276 | `c761c1bba7e9e170fd0feb7d8d140bd17bf92cb9a498d3d020b13282defe345e` |
| `data.cpk::system/tool/effect/sample/I_SPARK0.EP` | EP | 17824 | `db38d9a0303f210cdec61d05050fccddd880ea346445b5201ef6363595bfe802` |
| `data.cpk::system/tool/effect/sample/I_SPHRE.EP` | EP | 292 | `ad2448363ac52bacafeebef3875ce8969171f516f77803c650adde320e2af06d` |
| `data.cpk::system/tool/effect/sample/I_SPHRE0.EP` | EP | 17824 | `12b03cebca119360b94b6a00caf57c09a22fc245381b27e5a8a6355cac4622dc` |
| `data.cpk::system/tool/effect/sample/I_SPIRL.EP` | EP | 292 | `aa8f88268ec0a54d5eca9b96e4ae094a9912c2d91e945a5f4d8db694ae0620f2` |
| `data.cpk::system/tool/effect/sample/I_SPIRL0.EP` | EP | 17840 | `efb49cb93b8ac77cc13c0e9251dd6d100b1b36a3b9afe4c36bc1122145ccaf54` |
| `data.cpk::system/tool/effect/sample/K_ASPAK0.EP` | EP | 612 | `5d7e3dcfdaae51f428c236de1c18e7edc51674c1248fad5555637e7f3f140826` |
| `data.cpk::system/tool/effect/sample/K_ATOM0.EP` | EP | 596 | `604feb36679b50a23c6670824648df24a94ad69fcc84677f0f74c6891a63c83d` |
| `data.cpk::system/tool/effect/sample/K_CSPAK0.EP` | EP | 612 | `b7618aba4b33993b7da3cde2be25c5ed47e26248ea02bfc10d41db73eaa4e5f0` |
| `data.cpk::system/tool/effect/sample/K_HSPAK0.EP` | EP | 596 | `ffeccc4b0f55c6ecc6a99b3b160d805849cdc98d16a6e34fbb1d32e691a8cbac` |
| `data.cpk::system/tool/effect/sample/K_RSPHR0.EP` | EP | 612 | `fc62f388ba77e0dabcc62f8d7109146959f6d24b21280b9bb5eae7de3acf78d4` |
| `data.cpk::system/tool/effect/sample/K_SMOKE0.EP` | EP | 324 | `dbff9bab151cc8d2c6fb018e489ba4ecd2518dc5b57489e7aeaa53f6115154a9` |
| `data.cpk::system/tool/effect/sample/K_SPARK0.EP` | EP | 596 | `91813daa9593b28cf2e5139c60c6da9bc361065a50a61ce273f5d4cd648db20f` |
| `data.cpk::system/tool/effect/sample/K_SPHRE0.EP` | EP | 596 | `be665d46da0734f98af2ab66fad36cb022f46ffdf4b5d68277fbc101decdf277` |
| `data.cpk::system/tool/effect/sample/K_SPIRL0.EP` | EP | 612 | `0c2fef678258937e7c4867cd3fb0e2cbe73b136fa1c618a4ccfd84c8b0e7147c` |
| `data.cpk::system/tool/effect/sample/L_PARA0.EP` | EP | 212 | `6747ab9cd1c4262ee9d8701263af3d077954891f8b0562dcacdffd88f4b7fd74` |
| `data.cpk::system/tool/effect/sample/L_RADI0.EP` | EP | 196 | `4acc54b9de482adbf0653f82bab31478681581bd3c9118d90dbfb7710e16ee46` |
| `data.cpk::system/tool/effect/sample/M_ASPAK0.EP` | EP | 12680 | `1d140b001dd9917d28b23370c481e3a4e99a26df52b28ca308d9249ace2a658c` |
| `data.cpk::system/tool/effect/sample/M_ATOM0.EP` | EP | 12680 | `9388ff569cb13ecce56c4ea4ac577db5030add21669eb3311ff456ea6ddcbc00` |
| `data.cpk::system/tool/effect/sample/M_CSMOK0.EP` | EP | 12712 | `816362b36800b1984f81209a5ded250f32d52affb889434d05f572ee2917921f` |
| `data.cpk::system/tool/effect/sample/M_CSPAK0.EP` | EP | 12696 | `b911d32103892768c480e1c6c792eb66037432f889483127aca56c719c2a2005` |
| `data.cpk::system/tool/effect/sample/M_HSPAK0.EP` | EP | 12680 | `f1fdddf704ed8b92f6fde57d20b176e87f0f2ec6760bfe6602d4c43086f1cd59` |
| `data.cpk::system/tool/effect/sample/M_RSPHR0.EP` | EP | 12696 | `768224d9515d39fd8c22d3cc729dbf96422e0a38dc015a6976ded59b6c7dc0a1` |
| `data.cpk::system/tool/effect/sample/M_SMOKE0.EP` | EP | 12680 | `c5fd8deef6b22d613e61dd16305496109cc3ae71263386129612afe2b55b99dc` |
| `data.cpk::system/tool/effect/sample/M_SPARK0.EP` | EP | 12664 | `ce05c67c794321b1e42e3a2067e70e3e620bf79446821fa6529ac9184af5ff97` |
| `data.cpk::system/tool/effect/sample/M_SPHRE0.EP` | EP | 12680 | `8f84090e220420bcc6debfa08ef03e5b58a4d40dc8d44a845d134f333650c0c5` |
| `data.cpk::system/tool/effect/sample/M_SPIRL0.EP` | EP | 12696 | `1cc6787c59575fc1fc63d54b9471a286f70e7e12990cb40fe80f663ca4df2309` |
| `data.cpk::system/tool/effect/sample/N_CYLDR0.EP` | EP | 292 | `463a045fec23b332c2402c7284fe350e151f7d8980d23d8dc1e1ef07a1b450c5` |
| `data.cpk::system/tool/effect/sample/N_CYLND0.EP` | EP | 292 | `d85ae863daec66dc3feaf56be102da2a9cebd5f9b8a3a0b809ee257c7fb05fd3` |
| `data.cpk::system/tool/effect/sample/N_CYLND1.EP` | EP | 292 | `248cdf255d32107109fbf09a4aa995df9040bd71c159f1027bdfadb0caea8e80` |
| `data.cpk::system/tool/effect/sample/N_CYLND2.EP` | EP | 292 | `82c3135df7f509ab4fc7d7d2659e6465e457eb629eed7e38d76b166d6f053a02` |
| `data.cpk::system/tool/effect/sample/N_SPHRE0.EP` | EP | 276 | `2a55eb6ac55ac85f152bc631be582f442b1cd8f4f4516af0ebe13e7109c2146b` |
| `data.cpk::system/tool/effect/sample/N_SPIRL0.EP` | EP | 308 | `50ffde89469bb345f1c3bf211811113553b029a37bc3dbdac95ac02f6f058e03` |
| `data.cpk::system/tool/effect/sample/N_SPIRL1.EP` | EP | 308 | `d2134a28cf4ca8311e8f197b2585818d3b25633b025586bc4765a169d236fa12` |
| `data.cpk::system/tool/effect/sample/N_SPIRL2.EP` | EP | 308 | `17d886642c1deaaee859e3fc2497fbedb62ceadb1e87cd66fdf3e369b320128e` |
| `data.cpk::system/tool/effect/sample/O_DSHOCK0.EP` | EP | 100 | `447d566d63a53b28d482649a5caa5fbeb80ca194ffb184fa4b16c29105493267` |
| `data.cpk::system/tool/effect/sample/O_EFF_L0.EP` | EP | 68 | `ef90fd56fc8fd9605719b097c811f74e368f55d1260f3d52e7bfe9cba527675d` |
| `data.cpk::system/tool/effect/sample/O_FLD_F0.EP` | EP | 68 | `785e7cb9360d9de6607235be76f9a115e7a1499be4a423c2e3c586da0f578ada` |
| `data.cpk::system/tool/effect/sample/O_FLD_L0.EP` | EP | 68 | `4fa2fa2b002dd8a809739cd61191a298f73d8bf720b3037533487394e8741759` |
| `data.cpk::system/tool/effect/sample/O_FOVY0.EP` | EP | 164 | `663c8c4d0371817d1524d9f68adcde0fa3ed09906b3b12504799e520aa960fb7` |
| `data.cpk::system/tool/effect/sample/O_HLP_CM0.EP` | EP | 2332 | `1c8517ee7c6ac0c97cde507099a855a94ff2055ee98d4507a162b291fa8c159c` |
| `data.cpk::system/tool/effect/sample/O_HLP_UT0.EP` | EP | 1432 | `9284d4317171cd7f64c5dd1baf9cec9d2aebf06ebaf8d8c162fb69baac6ebce3` |
| `data.cpk::system/tool/effect/sample/O_LCHAR0.EP` | EP | 68 | `3d9e8d163d920c221ee25b6e9e70a953ef371f41808802c1157129ffbc6ee682` |
| `data.cpk::system/tool/effect/sample/O_LEFF0.EP` | EP | 100 | `6d46910338adea185e63a5215e6c3a6b7d3686c5d8462f4d1821b6282835ff0c` |
| `data.cpk::system/tool/effect/sample/O_LFLD0.EP` | EP | 68 | `5ea6bb5374a197292107674cbfb3a595441861d674799708a0d4df89c8da2dfa` |
| `data.cpk::system/tool/effect/sample/O_LIGHT0.EP` | EP | 100 | `f0865f3a42750906b4f5ed4441a8d9b8c7a7ef133c79890f8fe345372dee3674` |
| `data.cpk::system/tool/effect/sample/O_LSKIL0.EP` | EP | 100 | `fff7f07a12e0a748ee30a1e6ec9c2ad5aefd84ae6937e69d6ebf075b51ec33d7` |
| `data.cpk::system/tool/effect/sample/O_MALPH0.EP` | EP | 84 | `6ad164b0cf8da7193e7659ddda54c65baf0ab85f3ce355a8709149abfbbb33fb` |
| `data.cpk::system/tool/effect/sample/O_MASKB0.EP` | EP | 132 | `2f88f957b357f8e6a9eff365a248afd50f88884bdee866720d74208beca07754` |
| `data.cpk::system/tool/effect/sample/O_MCHAR0.EP` | EP | 68 | `08bfaa95f86db18ec87fc434149dc5adcb2aa73139694784d3a160cdefa431d5` |
| `data.cpk::system/tool/effect/sample/O_MCOLR0.EP` | EP | 68 | `c039c5a5114a3908b752cc7366229c8f7dbd00aa712d516ba9c42fdd06ca7738` |
| `data.cpk::system/tool/effect/sample/O_MFLD0.EP` | EP | 68 | `8c9db121565c64cbeaa38e27b559b828923f910cb8fc2e948884c7eb2fb5012d` |
| `data.cpk::system/tool/effect/sample/O_MOT0.EP` | EP | 84 | `c542c07020975fd240328c7f69a04d7700db200adf957fdfa45f2e2ec78828ca` |
| `data.cpk::system/tool/effect/sample/O_P2_HT0.EP` | EP | 3088 | `cd3a090ae70da0a20475a6343050d34303eeb6979844c317ad469bcca418b21c` |
| `data.cpk::system/tool/effect/sample/O_P2_TND0.EP` | EP | 212 | `8966c0cd258e92068ff8a23919a9ab6215a9c9a4b2321616e3bcdae97f42224f` |
| `data.cpk::system/tool/effect/sample/O_P2_TRK0.EP` | EP | 388 | `05d486d14f5c77184b7a9710e36b95fbfec4cb18679812821276dac515545a8b` |
| `data.cpk::system/tool/effect/sample/O_P2_TRK1.EP` | EP | 72080 | `e54816abfd280057daa37ddbf9134c8d4fdf70f17a0dfcbe65392b102e01f080` |
| `data.cpk::system/tool/effect/sample/O_PLSCL0.EP` | EP | 16672 | `eec7be0f4cb0d88e6265bfd08329cc4a58f6de5166f72acd22c93b236d313a70` |
| `data.cpk::system/tool/effect/sample/O_PRG_CM0.EP` | EP | 100 | `eadce309f119e11cdc7ef509ddca5b9ed5ca13d9ff1ad054f240cefb10ff5900` |
| `data.cpk::system/tool/effect/sample/O_SE0.EP` | EP | 52 | `f7680eac79c47d33f7ecda3db9f6378148eb34a8f081e77b6451c36df5057881` |
| `data.cpk::system/tool/effect/sample/O_VIBE0.EP` | EP | 84 | `c15b9dabc45a473c4c0b6090c6dabdb58e1043331af76634e19a37a9a5833fec` |
| `data.cpk::system/tool/effect/sample/P_ASPAK0.EP` | EP | 292 | `d480d73d030a5d394845203a705e575986ed60960ea81a3aaf7c00756ee0319d` |
| `data.cpk::system/tool/effect/sample/P_ASPAK1.EP` | EP | 71984 | `c84bcacfbf78ef005b6e8c3a7c2fa95b5fd6d038ea0bd38d0c99eab08e29ef45` |
| `data.cpk::system/tool/effect/sample/P_ATOM0.EP` | EP | 292 | `bb09ffc3a31952b7b416616bb7509304a29aa7dcd30239058c8e75c8f575cc9e` |
| `data.cpk::system/tool/effect/sample/P_CSMOK0.EP` | EP | 324 | `721fa09e3f5b60cee195bb780ca73440681cb2f831d47530698111ff98b805a3` |
| `data.cpk::system/tool/effect/sample/P_CSPAK0.EP` | EP | 308 | `efffb386956801bc1810414aeb21bf4b887b64e230c8afeb9fbc4fe14e8479b8` |
| `data.cpk::system/tool/effect/sample/P_HSPAK0.EP` | EP | 292 | `fb2988c78e4c16da9502e1c015522b03db1cadb2d8b9f5ae411886ee4a4c936a` |
| `data.cpk::system/tool/effect/sample/P_HSPAK1.EP` | EP | 71984 | `b5afd1f03741f6ef3bb86e820e7521d7dac934f8014308a32452f41434b8536a` |
| `data.cpk::system/tool/effect/sample/P_RSPHR0.EP` | EP | 308 | `26fd6c43afeee4fce0ff30ec384fa9dce670c24244aaede98df541f9f9935ae3` |
| `data.cpk::system/tool/effect/sample/P_SMOKE0.EP` | EP | 292 | `5e78f3fbbcb5670f0ed0aef1da6eb043c77243b830a12b37a26f56ad07fa14e1` |
| `data.cpk::system/tool/effect/sample/P_SMOKE1.EP` | EP | 292 | `48753954c4ca2548b81ed20910cd4586aa30979349bec7c70c5e16ef9f70b47f` |
| `data.cpk::system/tool/effect/sample/P_SMOKE2.EP` | EP | 79104 | `bb5695e0afbbb282914529beae9f87d0b71a65d388a2aebc97142d2c3e521bbe` |
| `data.cpk::system/tool/effect/sample/P_SPARK0.EP` | EP | 276 | `67c00577e12dba0483e133900be6eb9477aa6f86382b85d10a92ecf2da558f5d` |
| `data.cpk::system/tool/effect/sample/P_SPARK1.EP` | EP | 276 | `13f9a3c34f228b8ec84cf8750f1409c61448a2474cbdd2acb069caed8fdef95b` |
| `data.cpk::system/tool/effect/sample/P_SPARK2.EP` | EP | 71968 | `470dabcd9cd6ffc0b489df14d48fb593079f541352e7c918508e3f6840b173f7` |
| `data.cpk::system/tool/effect/sample/P_SPHRE0.EP` | EP | 292 | `1e8e6b91774907f1fcd9717f7a44738e77960972cb46fb36f3907cbb92ff5859` |
| `data.cpk::system/tool/effect/sample/P_SPHRE1.EP` | EP | 71984 | `63868e80b5a7928268a58920b282c32c2b57c1483b5d10b97c10eeaa00b9fbcd` |
| `data.cpk::system/tool/effect/sample/P_SPIRL0.EP` | EP | 292 | `01679ae3d1b015909e8c9d273acd899ce8e8840ec02c2228aeade9ce2a5199bc` |
| `data.cpk::system/tool/effect/sample/P_SPIRL1.EP` | EP | 71984 | `a3f4498da546b188ad541d1e40ccd3743c41846c3c497f00730d32bfcbd97416` |
| `data.cpk::system/tool/effect/sample/R_CRCLE0.EP` | EP | 164 | `ca27bfc7b232e4b07f26cc476e07309a1f8ab43aebd2b7de749bf501c5600245` |
| `data.cpk::system/tool/effect/sample/R_CRCLE1.EP` | EP | 164 | `a968c3f229c94d661ecb7c878ccaa124a43be66665c4aba589af1a5a5f938754` |
| `data.cpk::system/tool/effect/sample/R_NOMAL0.EP` | EP | 164 | `ba8ce619bd1d98e05baf24896fa9de10f6bb4cb8f55235df252d44128ef58927` |
| `data.cpk::system/tool/effect/sample/R_NOMAL1.EP` | EP | 164 | `b45cc55197660a4df30307bb605a02787d19d7bff4e1a6cd3eba0f6f700f96cc` |
| `data.cpk::system/tool/effect/sample/R_TRACK0.EP` | EP | 260 | `0979368bf8056207dcdeebf216ebb2152015375ed9df691d25237f37b428ce54` |
| `data.cpk::system/tool/effect/sample/S_CYLND0.EP` | EP | 212 | `9ae2c7bf18142e2af3f94c0f8b31d6ecbcc62df14768629535bc9c202e4d6de2` |
| `data.cpk::system/tool/effect/sample/S_FLD_T0.EP` | EP | 3456 | `2cf15e1a49a83eab07d8cea5a3767cd4640772de9db9235373352b4a4849d1f1` |
| `data.cpk::system/tool/effect/sample/S_RADI0.EP` | EP | 180 | `8838953e2b9baa2b247a51b206c3ea9fc53b15bcf66f10e0640264ac3902aeed` |
| `data.cpk::system/tool/effect/sample/S_RADI1.EP` | EP | 180 | `4c1c6a156fd9320c13057017febe627b025b03a1eb56614d931a164a302c8335` |
| `data.cpk::system/tool/effect/sample/S_RAD_T0.EP` | EP | 180 | `3b5b2e7ca57c647be4ee839283e0257167798fc177108b460d86bbb9476026a7` |
| `data.cpk::system/tool/effect/sample/S_RING0.EP` | EP | 228 | `7cb16e2c544235dd5b13354bab6770e11d0d536b573eea8f39858937cc9513c2` |
| `data.cpk::system/tool/effect/sample/S_RNG_T0.EP` | EP | 212 | `ba11ac46d7747dbc9fab6b49aba2e42861c2ec43dd0d3f436a4504b48bdfb555` |
| `data.cpk::system/tool/effect/sample/S_SCT_T0.EP` | EP | 212 | `d142f2e067cfb7eca650f2dd251aec9a6f5c7db2141eb0d956d7bf6017937f4a` |
| `data.cpk::system/tool/effect/sample/S_SPARK0.EP` | EP | 196 | `06e671f47a3d65061734164606c799691a0f7a7101d844d81dbab5236db85c56` |
| `data.cpk::system/tool/effect/sample/S_SPARK1.EP` | EP | 196 | `fd0d5d5a7080de8369c0a76d4492369489a1641a65863e8a9464c71b933b2753` |
| `data.cpk::system/tool/effect/sample/S_SPK_T0.EP` | EP | 196 | `5cea16c3c77496762b8c0943ebe3e0b4d6fdc060263b5ef255482063033da93e` |
| `data.cpk::system/tool/effect/sample/S_SPK_T1.EP` | EP | 196 | `5c16d7ce03aed3feabb0e2acd2fe9fd38f15aaf0fdb3a959fe310994e33ff970` |
| `data.cpk::system/tool/effect/sample/T_BOARD0.EP` | EP | 66832 | `d46d58b10cd9a298f80d74aa67ee1d78e4551a285c087956b26fc01520ca4b26` |
| `data.cpk::system/tool/effect/sample/U_CYLND0.EP` | EP | 324 | `0a4417b567e5b336757664408a27103b6a7385234c9b15025d36899beed67657` |
| `data.cpk::system/tool/effect/sample/U_SMOKE0.EP` | EP | 356 | `2005f071ef145f890956828e6f1e5e25f1c6861a9ad761f049dc138a3ccd656e` |
| `data.cpk::system/tool/effect/sample/U_SPHRE0.EP` | EP | 212 | `d72a0fd47112b349939fb3dc39ffaf78153b0e229142c7f856435242a89e58f1` |
| `data.cpk::system/tool/effect/sample/U_SPIRL0.EP` | EP | 356 | `9d953971619a25860e8d2f12d9ec98a92faf8a0b87235ee6c03916d3912d1122` |
| `data.cpk::system/tool/effect/sample/W_HSPAK0.EP` | EP | 324 | `2447ae805d2e53c295294a6a61f01be908b6a03313648585fbd628e133cdc3c0` |
| `data.cpk::system/tool/effect/sample/W_SMOKE0.EP` | EP | 324 | `bb0a57e464864048940292288a569cfa82431ae76e7ed3abf4554fcb67319f5a` |
| `data.cpk::system/tool/effect/sample/W_SPARK0.EP` | EP | 308 | `be0437acf68174027ece48a70687101ec2c39a78220bb689e43bec428bfd8869` |
| `data.cpk::system/tool/effect/sample/W_SPHRE0.EP` | EP | 324 | `3fe63ba7a4ea7569380abad37951501aa33c2f2b22fc8ff50daada9647594267` |
| `data.cpk::system/tool/effect/sample/W_SPIRL0.EP` | EP | 324 | `2cf936dd67e1993b1a333c73ec88984b831cc2dba8a816044479d2843a6fd7ef` |
| `data.cpk::system/tool/effect/sample/X_NOMAL0.EP` | EP | 5392 | `f7478b9c7046c1e9770f55b6ce0fb9919aee2a1bf44c7b3928d30020bb749a81` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::ATC_D_J.BED` | BED400 | 4448 | `f9ffa0abff120b243b99b3d6006697becb10735fc24f0fb798beab2a467ece7a` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::ATC_J_J.BED` | BED400 | 3616 | `b70ab3abd0d5491ba55fc2cb9ddfc918225ba0090e58e3aed9d396e628ce6dd3` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 6560 | `fa971120bb56cc962a0792d7af2a572779c23fa18769b814c5a531dc7e09f97f` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D_J.BED` | BED400 | 4448 | `f9ffa0abff120b243b99b3d6006697becb10735fc24f0fb798beab2a467ece7a` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J_J.BED` | BED400 | 3616 | `b70ab3abd0d5491ba55fc2cb9ddfc918225ba0090e58e3aed9d396e628ce6dd3` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 6560 | `fa971120bb56cc962a0792d7af2a572779c23fa18769b814c5a531dc7e09f97f` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_c.cpk::battle/boss/e200.bin::BED/e200_ENCOUNT.BED` | BED400 | 14288 | `4294f933ecf65bfba4e1b444999c903c3c47a29462602e79b53ed7a2e7ceb452` |
| `data_c.cpk::battle/boss/e200.bin::BED/e200_END.BED` | BED400 | 33088 | `931f2c31d66a9853504d6ad85b72593d9e62da352f97719a930c8bed0e3ab733` |
| `data_c.cpk::battle/boss/e200.bin::BED/e200_H160.BED` | BED400 | 9584 | `1a0d373c01cb5744cd98f6268da56e265539fe9b10a18a20d880348c77bc727a` |
| `data_c.cpk::battle/boss/e200.bin::BED/e200_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_c.cpk::battle/boss/e200.bin::BED/e200_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e200.bin::BED/e200_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e200.bin::BED/e200_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e200.bin::BED/e200_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e201.bin::BED/e201_ENCOUNT.BED` | BED400 | 10080 | `cf6ee31d878f652f19a197d9b18502f5e32e4c765131066088aa7a008feda7bb` |
| `data_c.cpk::battle/boss/e201.bin::BED/e201_END.BED` | BED400 | 30192 | `b31d78e5de0092924773b1d1b2e55a1955a77c1e2175c64ea794dba20d1c32fd` |
| `data_c.cpk::battle/boss/e201.bin::BED/e201_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_c.cpk::battle/boss/e201.bin::BED/e201_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e201.bin::BED/e201_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e201.bin::BED/e201_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e201.bin::BED/e201_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e202.bin::BED/e202_ENCOUNT.BED` | BED400 | 7344 | `5802ab6fc50884bbc4e1db4f97b92b60079582f1126c2ad60cc8e44567c4fc8c` |
| `data_c.cpk::battle/boss/e202.bin::BED/e202_END.BED` | BED400 | 30992 | `28078663ea649edae5bcd52b50f6e614bfb54f41f722d5d18654ea082fe50bc6` |
| `data_c.cpk::battle/boss/e202.bin::BED/e202_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_c.cpk::battle/boss/e202.bin::BED/e202_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e202.bin::BED/e202_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e202.bin::BED/e202_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e202.bin::BED/e202_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e203.bin::BED/e203_ENCOUNT.BED` | BED400 | 8960 | `97081976c63beb477742d9e10a2a83bf02c08fbac7c5e5a0c727b332304cf803` |
| `data_c.cpk::battle/boss/e203.bin::BED/e203_END.BED` | BED400 | 35536 | `6221812dc3626928a9ab734835fce955f56d3164995195624b7d20907e249863` |
| `data_c.cpk::battle/boss/e203.bin::BED/e203_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_c.cpk::battle/boss/e203.bin::BED/e203_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e203.bin::BED/e203_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e203.bin::BED/e203_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e203.bin::BED/e203_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_c.cpk::battle/boss/e203.bin::BED/e203_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e204.bin::BED/e204_ENCOUNT.BED` | BED400 | 9152 | `2b707e32159c57f61dda2a51f9a4d1bb13dbaff059eb0b294294771e29b2c155` |
| `data_c.cpk::battle/boss/e204.bin::BED/e204_END.BED` | BED400 | 7040 | `ff3be583ba835a33b94a87a47080141ccda1aaa9cbe5946ce80a861386da0534` |
| `data_c.cpk::battle/boss/e204.bin::BED/e204_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_c.cpk::battle/boss/e204.bin::BED/e204_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e204.bin::BED/e204_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e204.bin::BED/e204_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e204.bin::BED/e204_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_c.cpk::battle/boss/e204.bin::BED/e204_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e205.bin::BED/e205_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e205.bin::BED/e205_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e205.bin::BED/e205_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e205.bin::BED/e205_ENCOUNT.BED` | BED400 | 10336 | `2f5fad7c4101340a9f82380c62dc0341cf2218734ebcfb157e4f68be0eca017b` |
| `data_c.cpk::battle/boss/e205.bin::BED/e205_END.BED` | BED400 | 32320 | `151da6e1d4d39174971a53d45f228d2ef3e60e9513815541c9b7dc0e03187fe1` |
| `data_c.cpk::battle/boss/e205.bin::BED/e205_HATUDO.BED` | BED400 | 3696 | `18feb0b9a37f5d925e6a17a49030adb5b2280a2cb28fb6261a6dd1460507cdb0` |
| `data_c.cpk::battle/boss/e205.bin::BED/e205_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e205.bin::BED/e205_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e205.bin::BED/e205_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e205.bin::BED/e205_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_c.cpk::battle/boss/e205.bin::BED/e205_KAIJYO.BED` | BED400 | 5152 | `99bfb77d8d42d84c48c89e01526da14dd68075d99521a820c0675e0450e1d373` |
| `data_c.cpk::battle/boss/e205.bin::BED/e205_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e206.bin::BED/e206_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e206.bin::BED/e206_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e206.bin::BED/e206_ENCOUNT.BED` | BED400 | 6400 | `5b9ca9679564c387f222fcd3aefbba7ca6b0719893a33f2f14f35e1d09b064ce` |
| `data_c.cpk::battle/boss/e206.bin::BED/e206_END.BED` | BED400 | 31504 | `ee15eb497f11a707b85ad4c857527ba2732eaf4f9936fce305bbb4056de1d85d` |
| `data_c.cpk::battle/boss/e206.bin::BED/e206_H172.BED` | BED400 | 7552 | `74a7944243d6c20da666245d7ba3d0e76b957cb06136a5de79bbf03cd9c5f1bb` |
| `data_c.cpk::battle/boss/e206.bin::BED/e206_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_c.cpk::battle/boss/e206.bin::BED/e206_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e206.bin::BED/e206_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e206.bin::BED/e206_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e206.bin::BED/e206_H_SP.BED` | BED400 | 4064 | `7007fbcdd175abbf881d3ae61fe12bed93872c08931f3e6b82aed5bfc83a2dc7` |
| `data_c.cpk::battle/boss/e206.bin::BED/e206_OFF.BED` | BED400 | 8208 | `3a6e572de45506664a54fa77e2f355094be47cc77a6b603306bad35f8b940175` |
| `data_c.cpk::battle/boss/e206.bin::BED/e206_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e207.bin::BED/e207_ENCOUNT.BED` | BED400 | 6944 | `e5db68919f29386aa0f2142e7da82e62bea35af0985694fb2a2ded112255088f` |
| `data_c.cpk::battle/boss/e207.bin::BED/e207_END.BED` | BED400 | 32096 | `cc1f9d2b5dec6f8e5b3f3fa82b5710399537f04c31bb09896f4cee7a11e8d383` |
| `data_c.cpk::battle/boss/e207.bin::BED/e207_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_c.cpk::battle/boss/e207.bin::BED/e207_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e207.bin::BED/e207_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e207.bin::BED/e207_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e207.bin::BED/e207_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e208.bin::BED/e208_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e208.bin::BED/e208_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e208.bin::BED/e208_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e208.bin::BED/e208_ENCOUNT.BED` | BED400 | 14032 | `2275fd23ee2bea212640b51f317d363e17a98e93db834cba7a104090bb652b60` |
| `data_c.cpk::battle/boss/e208.bin::BED/e208_END.BED` | BED400 | 32336 | `b4a227dc5312f1dc2e4df9c6c0e5c11216ee2c24a029b1b7a209b50757dd2bde` |
| `data_c.cpk::battle/boss/e208.bin::BED/e208_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_c.cpk::battle/boss/e208.bin::BED/e208_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e208.bin::BED/e208_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e208.bin::BED/e208_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e208.bin::BED/e208_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_c.cpk::battle/boss/e208.bin::BED/e208_SK17B.BED` | BED400 | 13328 | `0d0a8231a695a1e9497d98246dfc0c544a03b95f827f3a45adb2fd3ffeb7a704` |
| `data_c.cpk::battle/boss/e208.bin::BED/e208_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e209.bin::BED/e209_ENCOUNT.BED` | BED400 | 9392 | `80719c0c13a8fde7ff0e545957665da6cffd72631c30e8525e0962927f919266` |
| `data_c.cpk::battle/boss/e209.bin::BED/e209_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data_c.cpk::battle/boss/e209.bin::BED/e209_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_c.cpk::battle/boss/e209.bin::BED/e209_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_c.cpk::battle/boss/e209.bin::BED/e209_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_c.cpk::battle/boss/e209.bin::BED/e209_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_c.cpk::battle/boss/e209.bin::BED/e209_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20A.bin::BED/e20A_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20A.bin::BED/e20A_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20A.bin::BED/e20A_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20A.bin::BED/e20A_ENCOUNT.BED` | BED400 | 7248 | `7ce81d540666b51712f628a3ace38d282202ae8035cd6c497bcba877582a0d26` |
| `data_c.cpk::battle/boss/e20A.bin::BED/e20A_END.BED` | BED400 | 30736 | `0a7fd4451e4224bbd65059a54872ed290c3a637eff66b564a19a7cafb69e82ec` |
| `data_c.cpk::battle/boss/e20A.bin::BED/e20A_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_c.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20A.bin::BED/e20A_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_c.cpk::battle/boss/e20A.bin::BED/e20A_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20B.bin::BED/e20B_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20B.bin::BED/e20B_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20B.bin::BED/e20B_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT.BED` | BED400 | 7408 | `bfbcd65ffdff97966112ba29739ca11224d5130bd9e24793232706a5d78b80c3` |
| `data_c.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT2.BED` | BED400 | 6976 | `f14a2a742989b86d727615b805d73d35b92cfa2e98e4f1f2715b33c7850086aa` |
| `data_c.cpk::battle/boss/e20B.bin::BED/e20B_END.BED` | BED400 | 20400 | `fd46f5973ff92fc5d781a282fb094494df3e8a55809730bd91d623b8b55180d8` |
| `data_c.cpk::battle/boss/e20B.bin::BED/e20B_END1.BED` | BED400 | 52608 | `8069c2bcc904b6fa9ed727768508d39a56ecc95f3ce509fbb4d92d5c9c1073fb` |
| `data_c.cpk::battle/boss/e20B.bin::BED/e20B_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_c.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20B.bin::BED/e20B_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_c.cpk::battle/boss/e20B.bin::BED/e20B_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20C.bin::BED/e20C_ENCOUNT.BED` | BED400 | 8016 | `0d7531f757e8a743070f66980adba7097a34223c7ceab5b7e15e1006db2b0f76` |
| `data_c.cpk::battle/boss/e20C.bin::BED/e20C_END.BED` | BED400 | 23760 | `98e314cd795aabd6134413891b58afcd5b06a5fa4c2668ab5014bb7c9e19ec9f` |
| `data_c.cpk::battle/boss/e20C.bin::BED/e20C_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_c.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20C.bin::BED/e20C_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e20D.bin::BED/e20D_ENCOUNT.BED` | BED400 | 6592 | `d23955927d2d12aa710e56458ce8d64c9b172cedcea6c36cd99c5696fb1d049f` |
| `data_c.cpk::battle/boss/e20D.bin::BED/e20D_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data_c.cpk::battle/boss/e20D.bin::BED/e20D_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_c.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_c.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_c.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_c.cpk::battle/boss/e20D.bin::BED/e20D_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e214.bin::BED/e214_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e214.bin::BED/e214_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e214.bin::BED/e214_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e214.bin::BED/e214_ENCOUNT.BED` | BED400 | 10176 | `b56f07d78e20e9c4dbd1adc3f9c582035dd213c4a751d4a6df5889ddd7492a3d` |
| `data_c.cpk::battle/boss/e214.bin::BED/e214_END.BED` | BED400 | 5696 | `d109a2a14e0ee8b4fd4764e6c08aa8b78cb68e76651a42ac48c3638a0361576a` |
| `data_c.cpk::battle/boss/e214.bin::BED/e214_HATUDO.BED` | BED400 | 3504 | `b1792d9bd7867be710d47f27d81220d79495ec12d027191ddcd917f963e0ead6` |
| `data_c.cpk::battle/boss/e214.bin::BED/e214_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e214.bin::BED/e214_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e214.bin::BED/e214_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e214.bin::BED/e214_H_SP.BED` | BED400 | 3696 | `122de1e6d1504bf72f5f0fb267dc05aa3e80483c32cf8db7e8330feb9cfcce82` |
| `data_c.cpk::battle/boss/e214.bin::BED/e214_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e215.bin::BED/e215_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e215.bin::BED/e215_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e215.bin::BED/e215_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e215.bin::BED/e215_ENCOUNT.BED` | BED400 | 11296 | `e99453fa2913f5bbcad4d7c7b3ac29df97d5d35f5c84188af83b20bc5f4b35cf` |
| `data_c.cpk::battle/boss/e215.bin::BED/e215_END.BED` | BED400 | 1230864 | `c30afdd4e4cf82c8eebe4aa26faf5893a3427e5bd1595dd635f9aa63e89c9fc5` |
| `data_c.cpk::battle/boss/e215.bin::BED/e215_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_c.cpk::battle/boss/e215.bin::BED/e215_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e215.bin::BED/e215_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e215.bin::BED/e215_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e215.bin::BED/e215_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_c.cpk::battle/boss/e215.bin::BED/e215_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e216.bin::BED/e216_ENCOUNT.BED` | BED400 | 11200 | `1dcc41a7929352eb50ba07e44f68d24ed1c0a12e8cbe954e7cb75f735f1da34e` |
| `data_c.cpk::battle/boss/e216.bin::BED/e216_END.BED` | BED400 | 10576 | `49b1c46532ed9d3cc5a87e47736efd0ea4ec0584910f0d09d6b57ad1153d3292` |
| `data_c.cpk::battle/boss/e216.bin::BED/e216_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_c.cpk::battle/boss/e216.bin::BED/e216_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e216.bin::BED/e216_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e216.bin::BED/e216_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e216.bin::BED/e216_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_c.cpk::battle/boss/e216.bin::BED/e216_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/boss/e217.bin::BED/e217_ENCOUNT.BED` | BED400 | 6304 | `959dca87cf9853542ee07ada31ffde9a842967b3194b9a38fec7e0e912fac55c` |
| `data_c.cpk::battle/boss/e217.bin::BED/e217_END.BED` | BED400 | 60592 | `ea818bdc095b1797f4927266ae081698520a0b3e4b3f4c4d33693bfd57d28f82` |
| `data_c.cpk::battle/boss/e217.bin::BED/e217_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_c.cpk::battle/boss/e217.bin::BED/e217_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_c.cpk::battle/boss/e217.bin::BED/e217_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_c.cpk::battle/boss/e217.bin::BED/e217_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_c.cpk::battle/boss/e217.bin::BED/e217_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_c.cpk::battle/cutin/syoukan/SYKN_1.EPL` | EPL | 144336 | `26434f8e512a635e6015ad68637633ea8d0cc4572e822b46e5003ee5c1333cf3` |
| `data_c.cpk::battle/cutin/syoukan/SYKN_2.EPL` | EPL | 144432 | `ab3c4799776a43b07bf9dbb4ae839f87c5d2ace9270c419f9793dfd690f409e0` |
| `data_c.cpk::battle/cutin/syoukan/SYKN_3.EPL` | EPL | 144624 | `86ee77c0834af7bfd0ba3083fe75c6e5767ca5d52b0147deaa1545ff69f6eae1` |
| `data_c.cpk::battle/cutin/syoukan/SYKN_4.EPL` | EPL | 144848 | `27e4d5fa9e14b7a70a039a8661c4f478617fc51663f9a4261d0cafaf6908a963` |
| `data_c.cpk::battle/cutin/syoukan/SYKN_6.EPL` | EPL | 144400 | `de11427ae9239136204dcbc025a1396ffd025875a47bfde48b1cc8591c3bd4d9` |
| `data_c.cpk::battle/cutin/syoukan/SYKN_7.EPL` | EPL | 143472 | `617e49da072f4b562784ed367731fa98839e7ff531c0509924fb71a09fc331fd` |
| `data_c.cpk::battle/cutin/syoukan/SYKN_8.EPL` | EPL | 140368 | `ebaa26e8de1adee3664d90898b5b9d34809560757b64d372024b6f3ddef432ff` |
| `data_c.cpk::battle/cutin/syoukan/SYKN_E109.EPL` | EPL | 173104 | `f121cad23dcd1d39b2fe45b4aa37732a94512ef064b4f985d34a726818aa7350` |
| `data_c.cpk::battle/result/shuffle.arc::SHUFFLE01.EPL` | EPL | 6368 | `bb7e2d674c4b18778f6f7b02304159ee7a192b90f38406353bd32754207244b5` |
| `data_c.cpk::battle/result/shuffle.arc::SHUFF_01.EPL` | EPL | 8864 | `59627e77df24e98b1aec5cf22a9c47495e145505c6fea06b5b0429b3427a7fad` |
| `data_c.cpk::battle/result/shuffle.arc::SHUFF_02.EPL` | EPL | 9760 | `58c0739b468638264c6d3534bd2b2c6f5af07a3e6d878fc4104351a4f01b9dad` |
| `data_c.cpk::battle/result/shuffle.arc::SHUFF_03.EPL` | EPL | 14688 | `c50a3a6c14cb67013b24aa7f12833c401ef5ef7f5bb823ec4a45916b1d996782` |
| `data_c.cpk::battle/result/shuffle.arc::SHUFF_04.EPL` | EPL | 9568 | `2ac489ffa035960b485696279cdcdc702e512321dfec3c5de3e0533354ac27ef` |
| `data_c.cpk::battle/result/shuffle.arc::SHUFF_05.EPL` | EPL | 10144 | `1c02f0750e4a6e99932d04af21a9752ae50b80597201fbc7ce944bdde2b45432` |
| `data_c.cpk::battle/result/shuffle.arc::SHUFF_06.EPL` | EPL | 15104 | `a7756c94b0471c8bedc1447c499df3233be21f8992655205858fb1e97e7d7c96` |
| `data_c.cpk::battle/result/shuffle.arc::SHUFF_09.EPL` | EPL | 1600 | `7db6177b81d30997b063517caf3b0a1a1781a7363f3219940aaa1d3deef01bb4` |
| `data_c.cpk::battle/union/U15D_A.BED` | BED400 | 229136 | `bf70bea6d255daeff4ef444131c15e45091c85bc4e6344ff1620d298283d10cc` |
| `data_c.cpk::battle/union/U15E_A.BED` | BED400 | 262672 | `b1cb3137884d0155a5b2b10fbf2fa5c0a79c808cb9499c24c862986e16fba59b` |
| `data_c.cpk::battle/union/U15F_A.BED` | BED400 | 251840 | `bd8a7a2b11b045eb1c37fe24bdb438a382444e9521e3726c09952b7121c4bf94` |
| `data_c.cpk::event/fishing.bin::FLOAT_01.EPL` | EPL | 7232 | `3f7721ea180386dc79379572ea567e7521b2a470d9f76167aa71bbdebdb3f5d0` |
| `data_c.cpk::event/fishing.bin::FLOAT_WV.EPL` | EPL | 7264 | `db0358e93caaab0ccfe666c308ece5ff9acc4522ae95044fbc00be249dadb346` |
| `data_c.cpk::event/fishing.bin::SPRAY_01.EPL` | EPL | 11424 | `6229b4e3eae59c7f62ee785737578b3aba127205f979dd528d31222cb60d0be3` |
| `data_c.cpk::event/fishing.bin::SPRAY_02.EPL` | EPL | 4288 | `54cabd45d19febb6d5decaeb2ee9c081f8c34a1e3273c2db20449cc0fb637aa1` |
| `data_c.cpk::event/fishing.bin::SPRAY_03.EPL` | EPL | 23296 | `34836ad2132b20984fab062cf62efbae171cd5438d7d64d11d3dbe06661d15f3` |
| `data_c.cpk::event/fishing.bin::SPRAY_04.EPL` | EPL | 22880 | `9d383ff669c401e9ff458d0904761040d95cfbd90b64b6aef2bf04dded321238` |
| `data_c.cpk::event/fishing.bin::WAVE_01.EPL` | EPL | 512 | `e73f641168a4d70f2cfcb10cf9d4bbb71fcf1466208de78c682d22e8ba876bb3` |
| `data_c.cpk::event/fishing.bin::WAVE_02.EPL` | EPL | 512 | `e623b56cce128bbc917f276a6238991b9417971c74de892194acc4e17610a982` |
| `data_c.cpk::event/fishing.bin::WAVE_03.EPL` | EPL | 864 | `7431335642b1c77dc423964534743a10b1c9193279f2e36078cba7315e7a8fd4` |
| `data_c.cpk::event/fishing.bin::WAVE_04.EPL` | EPL | 82368 | `5975b6978e545c03916310565458748458e384874ecd227887f43dc0eb331c5f` |
| `data_c.cpk::event/fishing.bin::WAVE_05.EPL` | EPL | 14752 | `362d3e3b9d86e96cc408b2ac392838551848ecb27d829fe2996d1581b7132cfc` |
| `data_c.cpk::event/fishing.bin::WAVE_06.EPL` | EPL | 15232 | `7f2fc8f2c4b6baf6b48cecabdfb1ab1c09b3971cab53ea9933f4df36846d71f9` |
| `data_c.cpk::event_data/community.bin::CARD_30.EPL` | EPL | 267120 | `51a878d3a3d380b26abeab633ad5e075894c519e3fa3436cce36af0d81cc6c91` |
| `data_c.cpk::event_data/community.bin::CARD_EF04.EPL` | EPL | 153600 | `feb474459c1fb4b4d3b11ea541d82ea2673a32a14118275c43e0430a21ec9311` |
| `data_c.cpk::event_data/community.bin::CARD_EF06.EPL` | EPL | 134112 | `cb1ed7c4957fdbd26b0081735cb9b4c0c55587eeb604c5d868c972e738011621` |
| `data_c.cpk::event_data/community.bin::CARD_EF07.EPL` | EPL | 19440 | `2b55441af548b5b3c90bc1bad8253942d9f68fe7cfa2cdf75779d5f0dcbc86f4` |
| `data_c.cpk::event_data/community.bin::CMY01_02.EPL` | EPL | 7184 | `4d4eee955d10ab94f0672798d119f60466cb4437c3027c45ad8d13c1cd826b73` |
| `data_c.cpk::event_data/community.bin::CMY02_01.EPL` | EPL | 7232 | `b5561bee072cbe2532eb2829bf5c39fc91927f53fdf184e65a67b8d01afc5dfe` |
| `data_c.cpk::event_data/community.bin::CMY04_06.EPL` | EPL | 2816 | `1d99560e14f4d5304bad68a200e428f909d7b806466b1e504b7c8cb1a5b79800` |
| `data_c.cpk::event_data/community.bin::CMY05_05.EPL` | EPL | 13440 | `4252a6171efe301668d8d50dc48ad1d5157e8fb7ea0e10e1322c31690c4747e1` |
| `data_c.cpk::event_data/community.bin::E_KIRA.EPL` | EPL | 12560 | `9b83128cbed2ba53cb6c6bea178e6a26ba031a32db7e39f0863e09adac8b5e9c` |
| `data_c.cpk::event_data/community.bin::E_KIRA2.EPL` | EPL | 12560 | `cbc82cec6fd117a07c16d78d18265390c36c0ec058022e0f36fba5e944fabf85` |
| `data_c.cpk::event_data/community.bin::P_GET3.EPL` | EPL | 2128 | `928fd9aaecd4c39732a2c2492670dac71de00a6dda594d74c2316286a5127c9d` |
| `data_c.cpk::facility/cmbroot.arc::E_PER.EPL` | EPL | 1120 | `07b9bac782b287964bc963b576ddc16a1724ce44010e3bf9a86f6972a757a1a9` |
| `data_c.cpk::facility/cmbroot.arc::GAT07_01.EPL` | EPL | 19840 | `367b85f1f9785cb0a1aace1573b309b9a9eecf9df2d174eef2d78cdc332c8d16` |
| `data_c.cpk::facility/cmbroot.arc::PER3.EPL` | EPL | 640 | `a8420c4ae86d038a3f45da5fed5afb3bcc040783e1aa5372c1957916fb5bf9b8` |
| `data_c.cpk::field/SNOW_old.EPL` | EPL | 640 | `ff17b8ecac6518601e522bbc9eaf963fe6b3f480e82167891c228fcf9f8a2a99` |
| `data_c.cpk::field/pack/fd031_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_c.cpk::field/pack/fd031_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_c.cpk::field/pack/fd031_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_c.cpk::field/pack/fd031_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_c.cpk::field/pack/fd069_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_c.cpk::field/pack/fd069_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_c.cpk::field/pack/fd069_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_c.cpk::field/pack/fd069_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_c.cpk::field/pack/fd069_003.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_c.cpk::field/pack/fd069_003.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_c.cpk::field/pack/fd069_004.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_c.cpk::field/pack/fd069_004.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON1.EPL` | EPL | 35360 | `419957b5504a91136e2a6da0a9191adbb0d0fdcabec7745d7c6a0245337e1a0c` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON10.EPL` | EPL | 640 | `a459f3d6a49e217485c18351e1be0074790389f619fa11a21c41d10f817a629e` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON11.EPL` | EPL | 35360 | `a2b85ec8fd28435027137c00eb3db2613488890e426eb15950693e450ae3666e` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON12.EPL` | EPL | 13920 | `7c600dbf814ca52978843c10998ac5877290426bce5fc15b11d188ced6dc7655` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON13.EPL` | EPL | 2816 | `4c470d96fa640beb21f4b5c5f69da7278816df340150539b19a6d282b58d3745` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON14.EPL` | EPL | 3200 | `44511407c6c60dcb3fdffc2ea0b295f3e367373ad4e47fafb8f68ffbfa523742` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON15.EPL` | EPL | 3776 | `f376c7bd0d6fd7aa6614093b86b47465f5ce418e4456293015687a845851c6c0` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON16.EPL` | EPL | 8960 | `749535ddb03ac80e9c173a4e2c4e011bb445ef4c3afea446d9327356e03e53e8` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON17.EPL` | EPL | 5408 | `682ce6151e173a4e5d837be7324c60c7e5475112682d4f7734f32c2200316fa1` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON18.EPL` | EPL | 2816 | `ac6cae65586ea536872e38741e8d1b4f710faaa61209c23a54c2daf629f38528` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON19.EPL` | EPL | 10784 | `1712a4d006ad15b02ac959902c2ea753f852832ec6d94c459ca0dd92b2f31ed1` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON2.EPL` | EPL | 70560 | `d1f15c72717d7ae5b2b5b495c209f9ce6928d98bdd7a0a9e0d45e6c95ba8e12b` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON20.EPL` | EPL | 11456 | `3c36b0b747116189a73e67773c05f474611512d39db9f7a81c47ce562f40632e` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON21.EPL` | EPL | 16768 | `ca32d5e7033db22860116d6accd7983ace3ee5c840f8ef936f22566b987bca92` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON22.EPL` | EPL | 4448 | `d0d9ad27c09e15df5caccb17467b71e5b15ba39d20f2c325d4d445caa7aac979` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON23.EPL` | EPL | 7328 | `83a7d4ae49cc4d9237798ed7eb905ef31e237550537bd89f67cf5744fbf8d102` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON24.EPL` | EPL | 13440 | `91538d6b1c7770ad8a4b4a2c34556939c9c1e4f1fc1beede20f8ba87ac5d5e7a` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON25.EPL` | EPL | 52960 | `5d8963dec880d512cc7453b46e2c0187e4113a21f82a431656472a48467951e5` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON26.EPL` | EPL | 2816 | `37621fb4cae82626436d81fabdc9b4177905063ac5264703e7de25f58e06bad3` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON3.EPL` | EPL | 12928 | `c8174ba08c05c1d7c62cb57138c3487e376dcf16713d3341a6018fc413e46688` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON4.EPL` | EPL | 32928 | `2f173d83c1e9a766c6bd5f21b90812ba376f44e2c7fd4b314b3e8c3004087b64` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON5.EPL` | EPL | 10464 | `72c96b08b6fdf5fcc5aad1a3dc4961b30e063647d1c8bac2a51ec464789c7bdd` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON6.EPL` | EPL | 24064 | `9115e4344e6efa200a9b2bad425236d6c514a6f4109e1154b080140c0889cd1b` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON7.EPL` | EPL | 2816 | `dedee54a4f210c6a3e993d17ed78bbc896a6c1e97acafc1e50e21b7ceb78f149` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON8.EPL` | EPL | 5472 | `15038b74e6204b01c8f1fee6595dba30b926e4a37e052bc8de8f53b10fb77dec` |
| `data_c.cpk::init.bin::event/icon.bin::_ICON9.EPL` | EPL | 34688 | `f4970730444758d9b1e2c20933ecdba4d9c69a92422d4722eb33caf384c0491b` |
| `data_c.cpk::init.bin::field/F_BLUR.EPL` | EPL | 640 | `f7c0c9b76c0cc4cb11dbe0816d1d3d83ef72ccd00f33a9480acf0ecd55fcfb6c` |
| `data_c.cpk::init.bin::field/F_ICON01.EPL` | EPL | 11856 | `bb0d1adabff9e8808efb526a1d5ed9a438e00209958956d7fd8b83d811836882` |
| `data_c.cpk::init.bin::field/F_ICON02.EPL` | EPL | 11312 | `37486846f11641eae744873620fd47689a2b343adebd53a567287c1918007555` |
| `data_c.cpk::init.bin::field/F_ICON03.EPL` | EPL | 11952 | `9f58f431f3147529befe48618a378f464538847caf9d1ac7ec6cf0fb2a1c21c7` |
| `data_c.cpk::init.bin::field/F_ICON04.EPL` | EPL | 11312 | `a2b0a94439e8716ecece6426d24874563ad7d39294abd923b21c226ca890eeb7` |
| `data_c.cpk::init.bin::field/F_ICON05.EPL` | EPL | 11312 | `24aa952f33209549dd9a2c0ed01f2bf1f9e1f3d1883fcd1bdcdca959d4c827ca` |
| `data_c.cpk::init.bin::field/RAIN_01.EPL` | EPL | 9440 | `af7e203dbc36d37d53577a673f20b9a6b71fa5bd1b52189326d4ea476e560c5e` |
| `data_c.cpk::init.bin::field/SNOW_01.EPL` | EPL | 640 | `4c614a3d103ad5745d5610865f9e2cdac2fc4b90c04b343d1f3ff61401c0bba1` |
| `data_c.cpk::init.bin::field/SNOW_02.EPL` | EPL | 640 | `3f025061842c24fa748cdd09adabd7de493c95eae332e544557efd0602798454` |
| `data_c.cpk::init.bin::field/effect/BREATH_01.EPL` | EPL | 640 | `9e5da14b05e6803b490c3f6e71ceac80965aaba7184b3ce317229ecf53617630` |
| `data_c.cpk::init.bin::field/effect/FOOT_L.EPL` | EPL | 5728 | `e4169139debccf8f28e88e86e2b72a49e10ecfee15a61eb745fd1a1bf1f18eee` |
| `data_c.cpk::init.bin::field/effect/FOOT_R.EPL` | EPL | 5728 | `83828867762a5b9ed87f11eb45f91284974350052e85d8993a737fc0229e12a2` |
| `data_c.cpk::init.bin::insect/insect.bin::INSECT_01.EPL` | EPL | 9696 | `74be2671465a4e5e760c6697cd27f5e96dce151f48b488fc40262c75cda40ea7` |
| `data_c.cpk::init_free.bin::battle/effect/SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_c.cpk::skill/100.pac::100.BED` | BED400 | 197152 | `7f8de83b32825b23e5781c22e9d60829b4b52caedbd2e0b12758ac513c5052a2` |
| `data_c.cpk::skill/101.pac::101.BED` | BED400 | 207296 | `6829029f035cdbd5885d78bb7bbf6093a541d82b625e0bae21aa566dce3cbc5d` |
| `data_c.cpk::skill/102.pac::102.BED` | BED400 | 212416 | `af0afb7b99a5ac689b6e60ef7870aead498ad19dc4837245284de98de14a128d` |
| `data_c.cpk::skill/11C.pac::11C.BED` | BED400 | 430656 | `28af1b802d4ff23ba100669d1e3a7d122d1d71b1c81a49f929ed04a7151864a7` |
| `data_c.cpk::skill/172.pac::172.BED` | BED400 | 109824 | `4e6dbea9b498888bdff410af3fa69cbb08ec3739448505b58ccc11248c09db7b` |
| `data_c.cpk::skill/173.pac::173.BED` | BED400 | 163968 | `eb630c1e1764258f4984267a127ba078b3f25a4c94288d5e707db3edd94a5c4c` |
| `data_c.cpk::skill/174.pac::174.BED` | BED400 | 248736 | `f78e63bb66795e019b5c0594aede8a0d6b228880eb5f163a17b32728c7e0e856` |
| `data_c.cpk::skill/175.pac::175.BED` | BED400 | 248736 | `47bf47ba13f77b4d233538fbd440a226192d8cc1a590597d3be20cc5bf06ab59` |
| `data_c.cpk::skill/176.pac::176.BED` | BED400 | 248736 | `f6285ada612c320533732a753b116f94d1e3198a2b634a9e549c2a05a7a8469c` |
| `data_c.cpk::skill/177.pac::177.BED` | BED400 | 248736 | `dcd2369c24adcada1578fdfe155b5003374bff8c930183b97fa9514ceb1a4c70` |
| `data_c.cpk::skill/18E.pac::18E.BED` | BED400 | 212416 | `6dc1a8e243a4141870da4728961efe6dcc29ebb38de9e3c66ad47d69a84e483d` |
| `data_c.cpk::skill/18F.pac::18F.BED` | BED400 | 407104 | `8962ee7112d9fffb7b5c2d660438e4bbb15d71781af7f606947fc71d99e6011a` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::ATC_D_J.BED` | BED400 | 4448 | `f9ffa0abff120b243b99b3d6006697becb10735fc24f0fb798beab2a467ece7a` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::ATC_J_J.BED` | BED400 | 3616 | `b70ab3abd0d5491ba55fc2cb9ddfc918225ba0090e58e3aed9d396e628ce6dd3` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 6560 | `fa971120bb56cc962a0792d7af2a572779c23fa18769b814c5a531dc7e09f97f` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D_J.BED` | BED400 | 4448 | `f9ffa0abff120b243b99b3d6006697becb10735fc24f0fb798beab2a467ece7a` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J_J.BED` | BED400 | 3616 | `b70ab3abd0d5491ba55fc2cb9ddfc918225ba0090e58e3aed9d396e628ce6dd3` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 6560 | `fa971120bb56cc962a0792d7af2a572779c23fa18769b814c5a531dc7e09f97f` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_ck.cpk::battle/boss/e200.bin::BED/e200_ENCOUNT.BED` | BED400 | 14288 | `4294f933ecf65bfba4e1b444999c903c3c47a29462602e79b53ed7a2e7ceb452` |
| `data_ck.cpk::battle/boss/e200.bin::BED/e200_END.BED` | BED400 | 33088 | `931f2c31d66a9853504d6ad85b72593d9e62da352f97719a930c8bed0e3ab733` |
| `data_ck.cpk::battle/boss/e200.bin::BED/e200_H160.BED` | BED400 | 9584 | `1a0d373c01cb5744cd98f6268da56e265539fe9b10a18a20d880348c77bc727a` |
| `data_ck.cpk::battle/boss/e200.bin::BED/e200_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_ck.cpk::battle/boss/e200.bin::BED/e200_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e200.bin::BED/e200_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e200.bin::BED/e200_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e200.bin::BED/e200_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e201.bin::BED/e201_ENCOUNT.BED` | BED400 | 10080 | `cf6ee31d878f652f19a197d9b18502f5e32e4c765131066088aa7a008feda7bb` |
| `data_ck.cpk::battle/boss/e201.bin::BED/e201_END.BED` | BED400 | 30192 | `b31d78e5de0092924773b1d1b2e55a1955a77c1e2175c64ea794dba20d1c32fd` |
| `data_ck.cpk::battle/boss/e201.bin::BED/e201_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_ck.cpk::battle/boss/e201.bin::BED/e201_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e201.bin::BED/e201_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e201.bin::BED/e201_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e201.bin::BED/e201_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e202.bin::BED/e202_ENCOUNT.BED` | BED400 | 7344 | `5802ab6fc50884bbc4e1db4f97b92b60079582f1126c2ad60cc8e44567c4fc8c` |
| `data_ck.cpk::battle/boss/e202.bin::BED/e202_END.BED` | BED400 | 30992 | `28078663ea649edae5bcd52b50f6e614bfb54f41f722d5d18654ea082fe50bc6` |
| `data_ck.cpk::battle/boss/e202.bin::BED/e202_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_ck.cpk::battle/boss/e202.bin::BED/e202_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e202.bin::BED/e202_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e202.bin::BED/e202_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e202.bin::BED/e202_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e203.bin::BED/e203_ENCOUNT.BED` | BED400 | 8960 | `97081976c63beb477742d9e10a2a83bf02c08fbac7c5e5a0c727b332304cf803` |
| `data_ck.cpk::battle/boss/e203.bin::BED/e203_END.BED` | BED400 | 35536 | `6221812dc3626928a9ab734835fce955f56d3164995195624b7d20907e249863` |
| `data_ck.cpk::battle/boss/e203.bin::BED/e203_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_ck.cpk::battle/boss/e203.bin::BED/e203_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e203.bin::BED/e203_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e203.bin::BED/e203_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e203.bin::BED/e203_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_ck.cpk::battle/boss/e203.bin::BED/e203_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e204.bin::BED/e204_ENCOUNT.BED` | BED400 | 9152 | `2b707e32159c57f61dda2a51f9a4d1bb13dbaff059eb0b294294771e29b2c155` |
| `data_ck.cpk::battle/boss/e204.bin::BED/e204_END.BED` | BED400 | 7040 | `ff3be583ba835a33b94a87a47080141ccda1aaa9cbe5946ce80a861386da0534` |
| `data_ck.cpk::battle/boss/e204.bin::BED/e204_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_ck.cpk::battle/boss/e204.bin::BED/e204_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e204.bin::BED/e204_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e204.bin::BED/e204_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e204.bin::BED/e204_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_ck.cpk::battle/boss/e204.bin::BED/e204_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e205.bin::BED/e205_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e205.bin::BED/e205_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e205.bin::BED/e205_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e205.bin::BED/e205_ENCOUNT.BED` | BED400 | 10336 | `2f5fad7c4101340a9f82380c62dc0341cf2218734ebcfb157e4f68be0eca017b` |
| `data_ck.cpk::battle/boss/e205.bin::BED/e205_END.BED` | BED400 | 32320 | `151da6e1d4d39174971a53d45f228d2ef3e60e9513815541c9b7dc0e03187fe1` |
| `data_ck.cpk::battle/boss/e205.bin::BED/e205_HATUDO.BED` | BED400 | 3696 | `18feb0b9a37f5d925e6a17a49030adb5b2280a2cb28fb6261a6dd1460507cdb0` |
| `data_ck.cpk::battle/boss/e205.bin::BED/e205_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e205.bin::BED/e205_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e205.bin::BED/e205_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e205.bin::BED/e205_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_ck.cpk::battle/boss/e205.bin::BED/e205_KAIJYO.BED` | BED400 | 5152 | `99bfb77d8d42d84c48c89e01526da14dd68075d99521a820c0675e0450e1d373` |
| `data_ck.cpk::battle/boss/e205.bin::BED/e205_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e206.bin::BED/e206_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e206.bin::BED/e206_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e206.bin::BED/e206_ENCOUNT.BED` | BED400 | 6400 | `5b9ca9679564c387f222fcd3aefbba7ca6b0719893a33f2f14f35e1d09b064ce` |
| `data_ck.cpk::battle/boss/e206.bin::BED/e206_END.BED` | BED400 | 31504 | `ee15eb497f11a707b85ad4c857527ba2732eaf4f9936fce305bbb4056de1d85d` |
| `data_ck.cpk::battle/boss/e206.bin::BED/e206_H172.BED` | BED400 | 7552 | `74a7944243d6c20da666245d7ba3d0e76b957cb06136a5de79bbf03cd9c5f1bb` |
| `data_ck.cpk::battle/boss/e206.bin::BED/e206_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_ck.cpk::battle/boss/e206.bin::BED/e206_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e206.bin::BED/e206_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e206.bin::BED/e206_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e206.bin::BED/e206_H_SP.BED` | BED400 | 4064 | `7007fbcdd175abbf881d3ae61fe12bed93872c08931f3e6b82aed5bfc83a2dc7` |
| `data_ck.cpk::battle/boss/e206.bin::BED/e206_OFF.BED` | BED400 | 8208 | `3a6e572de45506664a54fa77e2f355094be47cc77a6b603306bad35f8b940175` |
| `data_ck.cpk::battle/boss/e206.bin::BED/e206_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e207.bin::BED/e207_ENCOUNT.BED` | BED400 | 6944 | `e5db68919f29386aa0f2142e7da82e62bea35af0985694fb2a2ded112255088f` |
| `data_ck.cpk::battle/boss/e207.bin::BED/e207_END.BED` | BED400 | 32096 | `cc1f9d2b5dec6f8e5b3f3fa82b5710399537f04c31bb09896f4cee7a11e8d383` |
| `data_ck.cpk::battle/boss/e207.bin::BED/e207_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_ck.cpk::battle/boss/e207.bin::BED/e207_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e207.bin::BED/e207_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e207.bin::BED/e207_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e207.bin::BED/e207_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e208.bin::BED/e208_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e208.bin::BED/e208_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e208.bin::BED/e208_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e208.bin::BED/e208_ENCOUNT.BED` | BED400 | 14032 | `2275fd23ee2bea212640b51f317d363e17a98e93db834cba7a104090bb652b60` |
| `data_ck.cpk::battle/boss/e208.bin::BED/e208_END.BED` | BED400 | 32336 | `b4a227dc5312f1dc2e4df9c6c0e5c11216ee2c24a029b1b7a209b50757dd2bde` |
| `data_ck.cpk::battle/boss/e208.bin::BED/e208_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_ck.cpk::battle/boss/e208.bin::BED/e208_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e208.bin::BED/e208_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e208.bin::BED/e208_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e208.bin::BED/e208_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_ck.cpk::battle/boss/e208.bin::BED/e208_SK17B.BED` | BED400 | 13328 | `0d0a8231a695a1e9497d98246dfc0c544a03b95f827f3a45adb2fd3ffeb7a704` |
| `data_ck.cpk::battle/boss/e208.bin::BED/e208_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e209.bin::BED/e209_ENCOUNT.BED` | BED400 | 9392 | `80719c0c13a8fde7ff0e545957665da6cffd72631c30e8525e0962927f919266` |
| `data_ck.cpk::battle/boss/e209.bin::BED/e209_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data_ck.cpk::battle/boss/e209.bin::BED/e209_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_ck.cpk::battle/boss/e209.bin::BED/e209_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_ck.cpk::battle/boss/e209.bin::BED/e209_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_ck.cpk::battle/boss/e209.bin::BED/e209_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_ck.cpk::battle/boss/e209.bin::BED/e209_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20A.bin::BED/e20A_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20A.bin::BED/e20A_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20A.bin::BED/e20A_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20A.bin::BED/e20A_ENCOUNT.BED` | BED400 | 7248 | `7ce81d540666b51712f628a3ace38d282202ae8035cd6c497bcba877582a0d26` |
| `data_ck.cpk::battle/boss/e20A.bin::BED/e20A_END.BED` | BED400 | 30736 | `0a7fd4451e4224bbd65059a54872ed290c3a637eff66b564a19a7cafb69e82ec` |
| `data_ck.cpk::battle/boss/e20A.bin::BED/e20A_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_ck.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20A.bin::BED/e20A_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_ck.cpk::battle/boss/e20A.bin::BED/e20A_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20B.bin::BED/e20B_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20B.bin::BED/e20B_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20B.bin::BED/e20B_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT.BED` | BED400 | 7408 | `bfbcd65ffdff97966112ba29739ca11224d5130bd9e24793232706a5d78b80c3` |
| `data_ck.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT2.BED` | BED400 | 6976 | `f14a2a742989b86d727615b805d73d35b92cfa2e98e4f1f2715b33c7850086aa` |
| `data_ck.cpk::battle/boss/e20B.bin::BED/e20B_END.BED` | BED400 | 20400 | `fd46f5973ff92fc5d781a282fb094494df3e8a55809730bd91d623b8b55180d8` |
| `data_ck.cpk::battle/boss/e20B.bin::BED/e20B_END1.BED` | BED400 | 52608 | `8069c2bcc904b6fa9ed727768508d39a56ecc95f3ce509fbb4d92d5c9c1073fb` |
| `data_ck.cpk::battle/boss/e20B.bin::BED/e20B_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_ck.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20B.bin::BED/e20B_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_ck.cpk::battle/boss/e20B.bin::BED/e20B_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20C.bin::BED/e20C_ENCOUNT.BED` | BED400 | 8016 | `0d7531f757e8a743070f66980adba7097a34223c7ceab5b7e15e1006db2b0f76` |
| `data_ck.cpk::battle/boss/e20C.bin::BED/e20C_END.BED` | BED400 | 23760 | `98e314cd795aabd6134413891b58afcd5b06a5fa4c2668ab5014bb7c9e19ec9f` |
| `data_ck.cpk::battle/boss/e20C.bin::BED/e20C_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_ck.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20C.bin::BED/e20C_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e20D.bin::BED/e20D_ENCOUNT.BED` | BED400 | 6592 | `d23955927d2d12aa710e56458ce8d64c9b172cedcea6c36cd99c5696fb1d049f` |
| `data_ck.cpk::battle/boss/e20D.bin::BED/e20D_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data_ck.cpk::battle/boss/e20D.bin::BED/e20D_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_ck.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_ck.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_ck.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_ck.cpk::battle/boss/e20D.bin::BED/e20D_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e214.bin::BED/e214_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e214.bin::BED/e214_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e214.bin::BED/e214_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e214.bin::BED/e214_ENCOUNT.BED` | BED400 | 10176 | `b56f07d78e20e9c4dbd1adc3f9c582035dd213c4a751d4a6df5889ddd7492a3d` |
| `data_ck.cpk::battle/boss/e214.bin::BED/e214_END.BED` | BED400 | 5696 | `d109a2a14e0ee8b4fd4764e6c08aa8b78cb68e76651a42ac48c3638a0361576a` |
| `data_ck.cpk::battle/boss/e214.bin::BED/e214_HATUDO.BED` | BED400 | 3504 | `b1792d9bd7867be710d47f27d81220d79495ec12d027191ddcd917f963e0ead6` |
| `data_ck.cpk::battle/boss/e214.bin::BED/e214_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e214.bin::BED/e214_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e214.bin::BED/e214_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e214.bin::BED/e214_H_SP.BED` | BED400 | 3696 | `122de1e6d1504bf72f5f0fb267dc05aa3e80483c32cf8db7e8330feb9cfcce82` |
| `data_ck.cpk::battle/boss/e214.bin::BED/e214_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e215.bin::BED/e215_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e215.bin::BED/e215_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e215.bin::BED/e215_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e215.bin::BED/e215_ENCOUNT.BED` | BED400 | 11296 | `e99453fa2913f5bbcad4d7c7b3ac29df97d5d35f5c84188af83b20bc5f4b35cf` |
| `data_ck.cpk::battle/boss/e215.bin::BED/e215_END.BED` | BED400 | 1230864 | `c30afdd4e4cf82c8eebe4aa26faf5893a3427e5bd1595dd635f9aa63e89c9fc5` |
| `data_ck.cpk::battle/boss/e215.bin::BED/e215_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_ck.cpk::battle/boss/e215.bin::BED/e215_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e215.bin::BED/e215_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e215.bin::BED/e215_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e215.bin::BED/e215_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_ck.cpk::battle/boss/e215.bin::BED/e215_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e216.bin::BED/e216_ENCOUNT.BED` | BED400 | 11200 | `1dcc41a7929352eb50ba07e44f68d24ed1c0a12e8cbe954e7cb75f735f1da34e` |
| `data_ck.cpk::battle/boss/e216.bin::BED/e216_END.BED` | BED400 | 10576 | `49b1c46532ed9d3cc5a87e47736efd0ea4ec0584910f0d09d6b57ad1153d3292` |
| `data_ck.cpk::battle/boss/e216.bin::BED/e216_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_ck.cpk::battle/boss/e216.bin::BED/e216_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e216.bin::BED/e216_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e216.bin::BED/e216_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e216.bin::BED/e216_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_ck.cpk::battle/boss/e216.bin::BED/e216_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/boss/e217.bin::BED/e217_ENCOUNT.BED` | BED400 | 6304 | `959dca87cf9853542ee07ada31ffde9a842967b3194b9a38fec7e0e912fac55c` |
| `data_ck.cpk::battle/boss/e217.bin::BED/e217_END.BED` | BED400 | 60592 | `ea818bdc095b1797f4927266ae081698520a0b3e4b3f4c4d33693bfd57d28f82` |
| `data_ck.cpk::battle/boss/e217.bin::BED/e217_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_ck.cpk::battle/boss/e217.bin::BED/e217_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_ck.cpk::battle/boss/e217.bin::BED/e217_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_ck.cpk::battle/boss/e217.bin::BED/e217_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_ck.cpk::battle/boss/e217.bin::BED/e217_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_ck.cpk::battle/result/shuffle.arc::SHUFFLE01.EPL` | EPL | 6368 | `bb7e2d674c4b18778f6f7b02304159ee7a192b90f38406353bd32754207244b5` |
| `data_ck.cpk::battle/result/shuffle.arc::SHUFF_01.EPL` | EPL | 8864 | `59627e77df24e98b1aec5cf22a9c47495e145505c6fea06b5b0429b3427a7fad` |
| `data_ck.cpk::battle/result/shuffle.arc::SHUFF_02.EPL` | EPL | 9760 | `58c0739b468638264c6d3534bd2b2c6f5af07a3e6d878fc4104351a4f01b9dad` |
| `data_ck.cpk::battle/result/shuffle.arc::SHUFF_03.EPL` | EPL | 14688 | `c50a3a6c14cb67013b24aa7f12833c401ef5ef7f5bb823ec4a45916b1d996782` |
| `data_ck.cpk::battle/result/shuffle.arc::SHUFF_04.EPL` | EPL | 9568 | `2ac489ffa035960b485696279cdcdc702e512321dfec3c5de3e0533354ac27ef` |
| `data_ck.cpk::battle/result/shuffle.arc::SHUFF_05.EPL` | EPL | 10144 | `1c02f0750e4a6e99932d04af21a9752ae50b80597201fbc7ce944bdde2b45432` |
| `data_ck.cpk::battle/result/shuffle.arc::SHUFF_06.EPL` | EPL | 15104 | `a7756c94b0471c8bedc1447c499df3233be21f8992655205858fb1e97e7d7c96` |
| `data_ck.cpk::battle/result/shuffle.arc::SHUFF_09.EPL` | EPL | 1600 | `7db6177b81d30997b063517caf3b0a1a1781a7363f3219940aaa1d3deef01bb4` |
| `data_ck.cpk::event/fishing.bin::FLOAT_01.EPL` | EPL | 7232 | `3f7721ea180386dc79379572ea567e7521b2a470d9f76167aa71bbdebdb3f5d0` |
| `data_ck.cpk::event/fishing.bin::FLOAT_WV.EPL` | EPL | 7264 | `db0358e93caaab0ccfe666c308ece5ff9acc4522ae95044fbc00be249dadb346` |
| `data_ck.cpk::event/fishing.bin::SPRAY_01.EPL` | EPL | 11424 | `6229b4e3eae59c7f62ee785737578b3aba127205f979dd528d31222cb60d0be3` |
| `data_ck.cpk::event/fishing.bin::SPRAY_02.EPL` | EPL | 4288 | `54cabd45d19febb6d5decaeb2ee9c081f8c34a1e3273c2db20449cc0fb637aa1` |
| `data_ck.cpk::event/fishing.bin::SPRAY_03.EPL` | EPL | 23296 | `34836ad2132b20984fab062cf62efbae171cd5438d7d64d11d3dbe06661d15f3` |
| `data_ck.cpk::event/fishing.bin::SPRAY_04.EPL` | EPL | 22880 | `9d383ff669c401e9ff458d0904761040d95cfbd90b64b6aef2bf04dded321238` |
| `data_ck.cpk::event/fishing.bin::WAVE_01.EPL` | EPL | 512 | `e73f641168a4d70f2cfcb10cf9d4bbb71fcf1466208de78c682d22e8ba876bb3` |
| `data_ck.cpk::event/fishing.bin::WAVE_02.EPL` | EPL | 512 | `e623b56cce128bbc917f276a6238991b9417971c74de892194acc4e17610a982` |
| `data_ck.cpk::event/fishing.bin::WAVE_03.EPL` | EPL | 864 | `7431335642b1c77dc423964534743a10b1c9193279f2e36078cba7315e7a8fd4` |
| `data_ck.cpk::event/fishing.bin::WAVE_04.EPL` | EPL | 82368 | `5975b6978e545c03916310565458748458e384874ecd227887f43dc0eb331c5f` |
| `data_ck.cpk::event/fishing.bin::WAVE_05.EPL` | EPL | 14752 | `362d3e3b9d86e96cc408b2ac392838551848ecb27d829fe2996d1581b7132cfc` |
| `data_ck.cpk::event/fishing.bin::WAVE_06.EPL` | EPL | 15232 | `7f2fc8f2c4b6baf6b48cecabdfb1ab1c09b3971cab53ea9933f4df36846d71f9` |
| `data_ck.cpk::event_data/community.bin::CARD_30.EPL` | EPL | 267120 | `51a878d3a3d380b26abeab633ad5e075894c519e3fa3436cce36af0d81cc6c91` |
| `data_ck.cpk::event_data/community.bin::CARD_EF04.EPL` | EPL | 153600 | `feb474459c1fb4b4d3b11ea541d82ea2673a32a14118275c43e0430a21ec9311` |
| `data_ck.cpk::event_data/community.bin::CARD_EF06.EPL` | EPL | 134112 | `cb1ed7c4957fdbd26b0081735cb9b4c0c55587eeb604c5d868c972e738011621` |
| `data_ck.cpk::event_data/community.bin::CARD_EF07.EPL` | EPL | 19440 | `2b55441af548b5b3c90bc1bad8253942d9f68fe7cfa2cdf75779d5f0dcbc86f4` |
| `data_ck.cpk::event_data/community.bin::CMY01_02.EPL` | EPL | 7184 | `4d4eee955d10ab94f0672798d119f60466cb4437c3027c45ad8d13c1cd826b73` |
| `data_ck.cpk::event_data/community.bin::CMY02_01.EPL` | EPL | 7232 | `b5561bee072cbe2532eb2829bf5c39fc91927f53fdf184e65a67b8d01afc5dfe` |
| `data_ck.cpk::event_data/community.bin::CMY04_06.EPL` | EPL | 2816 | `1d99560e14f4d5304bad68a200e428f909d7b806466b1e504b7c8cb1a5b79800` |
| `data_ck.cpk::event_data/community.bin::CMY05_05.EPL` | EPL | 13440 | `4252a6171efe301668d8d50dc48ad1d5157e8fb7ea0e10e1322c31690c4747e1` |
| `data_ck.cpk::event_data/community.bin::E_KIRA.EPL` | EPL | 12560 | `9b83128cbed2ba53cb6c6bea178e6a26ba031a32db7e39f0863e09adac8b5e9c` |
| `data_ck.cpk::event_data/community.bin::E_KIRA2.EPL` | EPL | 12560 | `cbc82cec6fd117a07c16d78d18265390c36c0ec058022e0f36fba5e944fabf85` |
| `data_ck.cpk::event_data/community.bin::P_GET3.EPL` | EPL | 2128 | `928fd9aaecd4c39732a2c2492670dac71de00a6dda594d74c2316286a5127c9d` |
| `data_ck.cpk::facility/cmbroot.arc::E_PER.EPL` | EPL | 1120 | `07b9bac782b287964bc963b576ddc16a1724ce44010e3bf9a86f6972a757a1a9` |
| `data_ck.cpk::facility/cmbroot.arc::GAT07_01.EPL` | EPL | 19840 | `367b85f1f9785cb0a1aace1573b309b9a9eecf9df2d174eef2d78cdc332c8d16` |
| `data_ck.cpk::facility/cmbroot.arc::PER3.EPL` | EPL | 640 | `a8420c4ae86d038a3f45da5fed5afb3bcc040783e1aa5372c1957916fb5bf9b8` |
| `data_ck.cpk::field/pack/fd031_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_ck.cpk::field/pack/fd031_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_ck.cpk::field/pack/fd031_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_ck.cpk::field/pack/fd031_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_ck.cpk::field/pack/fd069_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_ck.cpk::field/pack/fd069_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_ck.cpk::field/pack/fd069_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_ck.cpk::field/pack/fd069_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_ck.cpk::field/pack/fd069_003.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_ck.cpk::field/pack/fd069_003.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_ck.cpk::field/pack/fd069_004.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_ck.cpk::field/pack/fd069_004.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON1.EPL` | EPL | 35360 | `419957b5504a91136e2a6da0a9191adbb0d0fdcabec7745d7c6a0245337e1a0c` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON10.EPL` | EPL | 640 | `a459f3d6a49e217485c18351e1be0074790389f619fa11a21c41d10f817a629e` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON11.EPL` | EPL | 35360 | `a2b85ec8fd28435027137c00eb3db2613488890e426eb15950693e450ae3666e` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON12.EPL` | EPL | 13920 | `7c600dbf814ca52978843c10998ac5877290426bce5fc15b11d188ced6dc7655` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON13.EPL` | EPL | 2816 | `4c470d96fa640beb21f4b5c5f69da7278816df340150539b19a6d282b58d3745` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON14.EPL` | EPL | 3200 | `44511407c6c60dcb3fdffc2ea0b295f3e367373ad4e47fafb8f68ffbfa523742` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON15.EPL` | EPL | 3776 | `f376c7bd0d6fd7aa6614093b86b47465f5ce418e4456293015687a845851c6c0` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON16.EPL` | EPL | 8960 | `749535ddb03ac80e9c173a4e2c4e011bb445ef4c3afea446d9327356e03e53e8` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON17.EPL` | EPL | 5408 | `682ce6151e173a4e5d837be7324c60c7e5475112682d4f7734f32c2200316fa1` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON18.EPL` | EPL | 2816 | `ac6cae65586ea536872e38741e8d1b4f710faaa61209c23a54c2daf629f38528` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON19.EPL` | EPL | 10784 | `1712a4d006ad15b02ac959902c2ea753f852832ec6d94c459ca0dd92b2f31ed1` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON2.EPL` | EPL | 70560 | `d1f15c72717d7ae5b2b5b495c209f9ce6928d98bdd7a0a9e0d45e6c95ba8e12b` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON20.EPL` | EPL | 11456 | `3c36b0b747116189a73e67773c05f474611512d39db9f7a81c47ce562f40632e` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON21.EPL` | EPL | 16768 | `ca32d5e7033db22860116d6accd7983ace3ee5c840f8ef936f22566b987bca92` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON22.EPL` | EPL | 4448 | `d0d9ad27c09e15df5caccb17467b71e5b15ba39d20f2c325d4d445caa7aac979` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON23.EPL` | EPL | 7328 | `83a7d4ae49cc4d9237798ed7eb905ef31e237550537bd89f67cf5744fbf8d102` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON24.EPL` | EPL | 13440 | `91538d6b1c7770ad8a4b4a2c34556939c9c1e4f1fc1beede20f8ba87ac5d5e7a` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON25.EPL` | EPL | 52960 | `5d8963dec880d512cc7453b46e2c0187e4113a21f82a431656472a48467951e5` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON26.EPL` | EPL | 2816 | `37621fb4cae82626436d81fabdc9b4177905063ac5264703e7de25f58e06bad3` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON3.EPL` | EPL | 12928 | `c8174ba08c05c1d7c62cb57138c3487e376dcf16713d3341a6018fc413e46688` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON4.EPL` | EPL | 32928 | `2f173d83c1e9a766c6bd5f21b90812ba376f44e2c7fd4b314b3e8c3004087b64` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON5.EPL` | EPL | 10464 | `72c96b08b6fdf5fcc5aad1a3dc4961b30e063647d1c8bac2a51ec464789c7bdd` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON6.EPL` | EPL | 24064 | `9115e4344e6efa200a9b2bad425236d6c514a6f4109e1154b080140c0889cd1b` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON7.EPL` | EPL | 2816 | `dedee54a4f210c6a3e993d17ed78bbc896a6c1e97acafc1e50e21b7ceb78f149` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON8.EPL` | EPL | 5472 | `15038b74e6204b01c8f1fee6595dba30b926e4a37e052bc8de8f53b10fb77dec` |
| `data_ck.cpk::init.bin::event/icon.bin::_ICON9.EPL` | EPL | 34688 | `f4970730444758d9b1e2c20933ecdba4d9c69a92422d4722eb33caf384c0491b` |
| `data_ck.cpk::init.bin::field/F_BLUR.EPL` | EPL | 640 | `f7c0c9b76c0cc4cb11dbe0816d1d3d83ef72ccd00f33a9480acf0ecd55fcfb6c` |
| `data_ck.cpk::init.bin::field/F_ICON01.EPL` | EPL | 11856 | `bb0d1adabff9e8808efb526a1d5ed9a438e00209958956d7fd8b83d811836882` |
| `data_ck.cpk::init.bin::field/F_ICON02.EPL` | EPL | 11312 | `37486846f11641eae744873620fd47689a2b343adebd53a567287c1918007555` |
| `data_ck.cpk::init.bin::field/F_ICON03.EPL` | EPL | 11952 | `9f58f431f3147529befe48618a378f464538847caf9d1ac7ec6cf0fb2a1c21c7` |
| `data_ck.cpk::init.bin::field/F_ICON04.EPL` | EPL | 11312 | `a2b0a94439e8716ecece6426d24874563ad7d39294abd923b21c226ca890eeb7` |
| `data_ck.cpk::init.bin::field/F_ICON05.EPL` | EPL | 11312 | `24aa952f33209549dd9a2c0ed01f2bf1f9e1f3d1883fcd1bdcdca959d4c827ca` |
| `data_ck.cpk::init.bin::field/RAIN_01.EPL` | EPL | 9440 | `af7e203dbc36d37d53577a673f20b9a6b71fa5bd1b52189326d4ea476e560c5e` |
| `data_ck.cpk::init.bin::field/SNOW_01.EPL` | EPL | 640 | `4c614a3d103ad5745d5610865f9e2cdac2fc4b90c04b343d1f3ff61401c0bba1` |
| `data_ck.cpk::init.bin::field/SNOW_02.EPL` | EPL | 640 | `3f025061842c24fa748cdd09adabd7de493c95eae332e544557efd0602798454` |
| `data_ck.cpk::init.bin::field/effect/BREATH_01.EPL` | EPL | 640 | `9e5da14b05e6803b490c3f6e71ceac80965aaba7184b3ce317229ecf53617630` |
| `data_ck.cpk::init.bin::field/effect/FOOT_L.EPL` | EPL | 5728 | `e4169139debccf8f28e88e86e2b72a49e10ecfee15a61eb745fd1a1bf1f18eee` |
| `data_ck.cpk::init.bin::field/effect/FOOT_R.EPL` | EPL | 5728 | `83828867762a5b9ed87f11eb45f91284974350052e85d8993a737fc0229e12a2` |
| `data_ck.cpk::init.bin::insect/insect.bin::INSECT_01.EPL` | EPL | 9696 | `74be2671465a4e5e760c6697cd27f5e96dce151f48b488fc40262c75cda40ea7` |
| `data_ck.cpk::init_free.bin::battle/effect/SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::ATC_D_J.BED` | BED400 | 4448 | `f9ffa0abff120b243b99b3d6006697becb10735fc24f0fb798beab2a467ece7a` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::ATC_J_J.BED` | BED400 | 3616 | `b70ab3abd0d5491ba55fc2cb9ddfc918225ba0090e58e3aed9d396e628ce6dd3` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 6560 | `fa971120bb56cc962a0792d7af2a572779c23fa18769b814c5a531dc7e09f97f` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D_J.BED` | BED400 | 4448 | `f9ffa0abff120b243b99b3d6006697becb10735fc24f0fb798beab2a467ece7a` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J_J.BED` | BED400 | 3616 | `b70ab3abd0d5491ba55fc2cb9ddfc918225ba0090e58e3aed9d396e628ce6dd3` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 6560 | `fa971120bb56cc962a0792d7af2a572779c23fa18769b814c5a531dc7e09f97f` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_de.cpk::battle/boss/e200.bin::BED/e200_ENCOUNT.BED` | BED400 | 14288 | `4294f933ecf65bfba4e1b444999c903c3c47a29462602e79b53ed7a2e7ceb452` |
| `data_de.cpk::battle/boss/e200.bin::BED/e200_END.BED` | BED400 | 33088 | `931f2c31d66a9853504d6ad85b72593d9e62da352f97719a930c8bed0e3ab733` |
| `data_de.cpk::battle/boss/e200.bin::BED/e200_H160.BED` | BED400 | 9584 | `1a0d373c01cb5744cd98f6268da56e265539fe9b10a18a20d880348c77bc727a` |
| `data_de.cpk::battle/boss/e200.bin::BED/e200_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_de.cpk::battle/boss/e200.bin::BED/e200_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e200.bin::BED/e200_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e200.bin::BED/e200_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e200.bin::BED/e200_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e201.bin::BED/e201_ENCOUNT.BED` | BED400 | 10080 | `cf6ee31d878f652f19a197d9b18502f5e32e4c765131066088aa7a008feda7bb` |
| `data_de.cpk::battle/boss/e201.bin::BED/e201_END.BED` | BED400 | 30192 | `b31d78e5de0092924773b1d1b2e55a1955a77c1e2175c64ea794dba20d1c32fd` |
| `data_de.cpk::battle/boss/e201.bin::BED/e201_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_de.cpk::battle/boss/e201.bin::BED/e201_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e201.bin::BED/e201_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e201.bin::BED/e201_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e201.bin::BED/e201_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e202.bin::BED/e202_ENCOUNT.BED` | BED400 | 7344 | `5802ab6fc50884bbc4e1db4f97b92b60079582f1126c2ad60cc8e44567c4fc8c` |
| `data_de.cpk::battle/boss/e202.bin::BED/e202_END.BED` | BED400 | 30992 | `28078663ea649edae5bcd52b50f6e614bfb54f41f722d5d18654ea082fe50bc6` |
| `data_de.cpk::battle/boss/e202.bin::BED/e202_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_de.cpk::battle/boss/e202.bin::BED/e202_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e202.bin::BED/e202_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e202.bin::BED/e202_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e202.bin::BED/e202_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e203.bin::BED/e203_ENCOUNT.BED` | BED400 | 8960 | `97081976c63beb477742d9e10a2a83bf02c08fbac7c5e5a0c727b332304cf803` |
| `data_de.cpk::battle/boss/e203.bin::BED/e203_END.BED` | BED400 | 35536 | `6221812dc3626928a9ab734835fce955f56d3164995195624b7d20907e249863` |
| `data_de.cpk::battle/boss/e203.bin::BED/e203_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_de.cpk::battle/boss/e203.bin::BED/e203_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e203.bin::BED/e203_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e203.bin::BED/e203_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e203.bin::BED/e203_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_de.cpk::battle/boss/e203.bin::BED/e203_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e204.bin::BED/e204_ENCOUNT.BED` | BED400 | 9152 | `2b707e32159c57f61dda2a51f9a4d1bb13dbaff059eb0b294294771e29b2c155` |
| `data_de.cpk::battle/boss/e204.bin::BED/e204_END.BED` | BED400 | 7040 | `ff3be583ba835a33b94a87a47080141ccda1aaa9cbe5946ce80a861386da0534` |
| `data_de.cpk::battle/boss/e204.bin::BED/e204_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_de.cpk::battle/boss/e204.bin::BED/e204_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e204.bin::BED/e204_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e204.bin::BED/e204_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e204.bin::BED/e204_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_de.cpk::battle/boss/e204.bin::BED/e204_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e205.bin::BED/e205_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e205.bin::BED/e205_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e205.bin::BED/e205_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e205.bin::BED/e205_ENCOUNT.BED` | BED400 | 10336 | `2f5fad7c4101340a9f82380c62dc0341cf2218734ebcfb157e4f68be0eca017b` |
| `data_de.cpk::battle/boss/e205.bin::BED/e205_END.BED` | BED400 | 32320 | `151da6e1d4d39174971a53d45f228d2ef3e60e9513815541c9b7dc0e03187fe1` |
| `data_de.cpk::battle/boss/e205.bin::BED/e205_HATUDO.BED` | BED400 | 3696 | `18feb0b9a37f5d925e6a17a49030adb5b2280a2cb28fb6261a6dd1460507cdb0` |
| `data_de.cpk::battle/boss/e205.bin::BED/e205_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e205.bin::BED/e205_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e205.bin::BED/e205_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e205.bin::BED/e205_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_de.cpk::battle/boss/e205.bin::BED/e205_KAIJYO.BED` | BED400 | 5152 | `99bfb77d8d42d84c48c89e01526da14dd68075d99521a820c0675e0450e1d373` |
| `data_de.cpk::battle/boss/e205.bin::BED/e205_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e206.bin::BED/e206_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e206.bin::BED/e206_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e206.bin::BED/e206_ENCOUNT.BED` | BED400 | 6400 | `5b9ca9679564c387f222fcd3aefbba7ca6b0719893a33f2f14f35e1d09b064ce` |
| `data_de.cpk::battle/boss/e206.bin::BED/e206_END.BED` | BED400 | 31504 | `ee15eb497f11a707b85ad4c857527ba2732eaf4f9936fce305bbb4056de1d85d` |
| `data_de.cpk::battle/boss/e206.bin::BED/e206_H172.BED` | BED400 | 7552 | `74a7944243d6c20da666245d7ba3d0e76b957cb06136a5de79bbf03cd9c5f1bb` |
| `data_de.cpk::battle/boss/e206.bin::BED/e206_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_de.cpk::battle/boss/e206.bin::BED/e206_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e206.bin::BED/e206_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e206.bin::BED/e206_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e206.bin::BED/e206_H_SP.BED` | BED400 | 4064 | `7007fbcdd175abbf881d3ae61fe12bed93872c08931f3e6b82aed5bfc83a2dc7` |
| `data_de.cpk::battle/boss/e206.bin::BED/e206_OFF.BED` | BED400 | 8208 | `3a6e572de45506664a54fa77e2f355094be47cc77a6b603306bad35f8b940175` |
| `data_de.cpk::battle/boss/e206.bin::BED/e206_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e207.bin::BED/e207_ENCOUNT.BED` | BED400 | 6944 | `e5db68919f29386aa0f2142e7da82e62bea35af0985694fb2a2ded112255088f` |
| `data_de.cpk::battle/boss/e207.bin::BED/e207_END.BED` | BED400 | 32096 | `cc1f9d2b5dec6f8e5b3f3fa82b5710399537f04c31bb09896f4cee7a11e8d383` |
| `data_de.cpk::battle/boss/e207.bin::BED/e207_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_de.cpk::battle/boss/e207.bin::BED/e207_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e207.bin::BED/e207_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e207.bin::BED/e207_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e207.bin::BED/e207_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e208.bin::BED/e208_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e208.bin::BED/e208_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e208.bin::BED/e208_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e208.bin::BED/e208_ENCOUNT.BED` | BED400 | 14032 | `2275fd23ee2bea212640b51f317d363e17a98e93db834cba7a104090bb652b60` |
| `data_de.cpk::battle/boss/e208.bin::BED/e208_END.BED` | BED400 | 32336 | `b4a227dc5312f1dc2e4df9c6c0e5c11216ee2c24a029b1b7a209b50757dd2bde` |
| `data_de.cpk::battle/boss/e208.bin::BED/e208_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_de.cpk::battle/boss/e208.bin::BED/e208_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e208.bin::BED/e208_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e208.bin::BED/e208_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e208.bin::BED/e208_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_de.cpk::battle/boss/e208.bin::BED/e208_SK17B.BED` | BED400 | 13328 | `0d0a8231a695a1e9497d98246dfc0c544a03b95f827f3a45adb2fd3ffeb7a704` |
| `data_de.cpk::battle/boss/e208.bin::BED/e208_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e209.bin::BED/e209_ENCOUNT.BED` | BED400 | 9392 | `80719c0c13a8fde7ff0e545957665da6cffd72631c30e8525e0962927f919266` |
| `data_de.cpk::battle/boss/e209.bin::BED/e209_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data_de.cpk::battle/boss/e209.bin::BED/e209_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_de.cpk::battle/boss/e209.bin::BED/e209_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_de.cpk::battle/boss/e209.bin::BED/e209_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_de.cpk::battle/boss/e209.bin::BED/e209_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_de.cpk::battle/boss/e209.bin::BED/e209_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20A.bin::BED/e20A_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20A.bin::BED/e20A_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20A.bin::BED/e20A_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20A.bin::BED/e20A_ENCOUNT.BED` | BED400 | 7248 | `7ce81d540666b51712f628a3ace38d282202ae8035cd6c497bcba877582a0d26` |
| `data_de.cpk::battle/boss/e20A.bin::BED/e20A_END.BED` | BED400 | 30736 | `0a7fd4451e4224bbd65059a54872ed290c3a637eff66b564a19a7cafb69e82ec` |
| `data_de.cpk::battle/boss/e20A.bin::BED/e20A_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_de.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20A.bin::BED/e20A_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_de.cpk::battle/boss/e20A.bin::BED/e20A_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20B.bin::BED/e20B_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20B.bin::BED/e20B_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20B.bin::BED/e20B_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT.BED` | BED400 | 7408 | `bfbcd65ffdff97966112ba29739ca11224d5130bd9e24793232706a5d78b80c3` |
| `data_de.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT2.BED` | BED400 | 6976 | `f14a2a742989b86d727615b805d73d35b92cfa2e98e4f1f2715b33c7850086aa` |
| `data_de.cpk::battle/boss/e20B.bin::BED/e20B_END.BED` | BED400 | 20400 | `fd46f5973ff92fc5d781a282fb094494df3e8a55809730bd91d623b8b55180d8` |
| `data_de.cpk::battle/boss/e20B.bin::BED/e20B_END1.BED` | BED400 | 52608 | `8069c2bcc904b6fa9ed727768508d39a56ecc95f3ce509fbb4d92d5c9c1073fb` |
| `data_de.cpk::battle/boss/e20B.bin::BED/e20B_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_de.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20B.bin::BED/e20B_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_de.cpk::battle/boss/e20B.bin::BED/e20B_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20C.bin::BED/e20C_ENCOUNT.BED` | BED400 | 8016 | `0d7531f757e8a743070f66980adba7097a34223c7ceab5b7e15e1006db2b0f76` |
| `data_de.cpk::battle/boss/e20C.bin::BED/e20C_END.BED` | BED400 | 23760 | `98e314cd795aabd6134413891b58afcd5b06a5fa4c2668ab5014bb7c9e19ec9f` |
| `data_de.cpk::battle/boss/e20C.bin::BED/e20C_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_de.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20C.bin::BED/e20C_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e20D.bin::BED/e20D_ENCOUNT.BED` | BED400 | 6592 | `d23955927d2d12aa710e56458ce8d64c9b172cedcea6c36cd99c5696fb1d049f` |
| `data_de.cpk::battle/boss/e20D.bin::BED/e20D_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data_de.cpk::battle/boss/e20D.bin::BED/e20D_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_de.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_de.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_de.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_de.cpk::battle/boss/e20D.bin::BED/e20D_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e214.bin::BED/e214_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e214.bin::BED/e214_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e214.bin::BED/e214_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e214.bin::BED/e214_ENCOUNT.BED` | BED400 | 10176 | `b56f07d78e20e9c4dbd1adc3f9c582035dd213c4a751d4a6df5889ddd7492a3d` |
| `data_de.cpk::battle/boss/e214.bin::BED/e214_END.BED` | BED400 | 5696 | `d109a2a14e0ee8b4fd4764e6c08aa8b78cb68e76651a42ac48c3638a0361576a` |
| `data_de.cpk::battle/boss/e214.bin::BED/e214_HATUDO.BED` | BED400 | 3504 | `b1792d9bd7867be710d47f27d81220d79495ec12d027191ddcd917f963e0ead6` |
| `data_de.cpk::battle/boss/e214.bin::BED/e214_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e214.bin::BED/e214_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e214.bin::BED/e214_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e214.bin::BED/e214_H_SP.BED` | BED400 | 3696 | `122de1e6d1504bf72f5f0fb267dc05aa3e80483c32cf8db7e8330feb9cfcce82` |
| `data_de.cpk::battle/boss/e214.bin::BED/e214_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e215.bin::BED/e215_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e215.bin::BED/e215_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e215.bin::BED/e215_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e215.bin::BED/e215_ENCOUNT.BED` | BED400 | 11296 | `e99453fa2913f5bbcad4d7c7b3ac29df97d5d35f5c84188af83b20bc5f4b35cf` |
| `data_de.cpk::battle/boss/e215.bin::BED/e215_END.BED` | BED400 | 1230864 | `c30afdd4e4cf82c8eebe4aa26faf5893a3427e5bd1595dd635f9aa63e89c9fc5` |
| `data_de.cpk::battle/boss/e215.bin::BED/e215_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_de.cpk::battle/boss/e215.bin::BED/e215_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e215.bin::BED/e215_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e215.bin::BED/e215_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e215.bin::BED/e215_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_de.cpk::battle/boss/e215.bin::BED/e215_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e216.bin::BED/e216_ENCOUNT.BED` | BED400 | 11200 | `1dcc41a7929352eb50ba07e44f68d24ed1c0a12e8cbe954e7cb75f735f1da34e` |
| `data_de.cpk::battle/boss/e216.bin::BED/e216_END.BED` | BED400 | 10576 | `49b1c46532ed9d3cc5a87e47736efd0ea4ec0584910f0d09d6b57ad1153d3292` |
| `data_de.cpk::battle/boss/e216.bin::BED/e216_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_de.cpk::battle/boss/e216.bin::BED/e216_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e216.bin::BED/e216_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e216.bin::BED/e216_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e216.bin::BED/e216_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_de.cpk::battle/boss/e216.bin::BED/e216_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/boss/e217.bin::BED/e217_ENCOUNT.BED` | BED400 | 6304 | `959dca87cf9853542ee07ada31ffde9a842967b3194b9a38fec7e0e912fac55c` |
| `data_de.cpk::battle/boss/e217.bin::BED/e217_END.BED` | BED400 | 60592 | `ea818bdc095b1797f4927266ae081698520a0b3e4b3f4c4d33693bfd57d28f82` |
| `data_de.cpk::battle/boss/e217.bin::BED/e217_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_de.cpk::battle/boss/e217.bin::BED/e217_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_de.cpk::battle/boss/e217.bin::BED/e217_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_de.cpk::battle/boss/e217.bin::BED/e217_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_de.cpk::battle/boss/e217.bin::BED/e217_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_de.cpk::battle/result/shuffle.arc::SHUFFLE01.EPL` | EPL | 6368 | `bb7e2d674c4b18778f6f7b02304159ee7a192b90f38406353bd32754207244b5` |
| `data_de.cpk::battle/result/shuffle.arc::SHUFF_01.EPL` | EPL | 8864 | `59627e77df24e98b1aec5cf22a9c47495e145505c6fea06b5b0429b3427a7fad` |
| `data_de.cpk::battle/result/shuffle.arc::SHUFF_02.EPL` | EPL | 9760 | `58c0739b468638264c6d3534bd2b2c6f5af07a3e6d878fc4104351a4f01b9dad` |
| `data_de.cpk::battle/result/shuffle.arc::SHUFF_03.EPL` | EPL | 14688 | `c50a3a6c14cb67013b24aa7f12833c401ef5ef7f5bb823ec4a45916b1d996782` |
| `data_de.cpk::battle/result/shuffle.arc::SHUFF_04.EPL` | EPL | 9568 | `2ac489ffa035960b485696279cdcdc702e512321dfec3c5de3e0533354ac27ef` |
| `data_de.cpk::battle/result/shuffle.arc::SHUFF_05.EPL` | EPL | 10144 | `1c02f0750e4a6e99932d04af21a9752ae50b80597201fbc7ce944bdde2b45432` |
| `data_de.cpk::battle/result/shuffle.arc::SHUFF_06.EPL` | EPL | 15104 | `a7756c94b0471c8bedc1447c499df3233be21f8992655205858fb1e97e7d7c96` |
| `data_de.cpk::battle/result/shuffle.arc::SHUFF_09.EPL` | EPL | 1600 | `7db6177b81d30997b063517caf3b0a1a1781a7363f3219940aaa1d3deef01bb4` |
| `data_de.cpk::event/fishing.bin::FLOAT_01.EPL` | EPL | 7232 | `3f7721ea180386dc79379572ea567e7521b2a470d9f76167aa71bbdebdb3f5d0` |
| `data_de.cpk::event/fishing.bin::FLOAT_WV.EPL` | EPL | 7264 | `db0358e93caaab0ccfe666c308ece5ff9acc4522ae95044fbc00be249dadb346` |
| `data_de.cpk::event/fishing.bin::SPRAY_01.EPL` | EPL | 11424 | `6229b4e3eae59c7f62ee785737578b3aba127205f979dd528d31222cb60d0be3` |
| `data_de.cpk::event/fishing.bin::SPRAY_02.EPL` | EPL | 4288 | `54cabd45d19febb6d5decaeb2ee9c081f8c34a1e3273c2db20449cc0fb637aa1` |
| `data_de.cpk::event/fishing.bin::SPRAY_03.EPL` | EPL | 23296 | `34836ad2132b20984fab062cf62efbae171cd5438d7d64d11d3dbe06661d15f3` |
| `data_de.cpk::event/fishing.bin::SPRAY_04.EPL` | EPL | 22880 | `9d383ff669c401e9ff458d0904761040d95cfbd90b64b6aef2bf04dded321238` |
| `data_de.cpk::event/fishing.bin::WAVE_01.EPL` | EPL | 512 | `e73f641168a4d70f2cfcb10cf9d4bbb71fcf1466208de78c682d22e8ba876bb3` |
| `data_de.cpk::event/fishing.bin::WAVE_02.EPL` | EPL | 512 | `e623b56cce128bbc917f276a6238991b9417971c74de892194acc4e17610a982` |
| `data_de.cpk::event/fishing.bin::WAVE_03.EPL` | EPL | 864 | `7431335642b1c77dc423964534743a10b1c9193279f2e36078cba7315e7a8fd4` |
| `data_de.cpk::event/fishing.bin::WAVE_04.EPL` | EPL | 82368 | `5975b6978e545c03916310565458748458e384874ecd227887f43dc0eb331c5f` |
| `data_de.cpk::event/fishing.bin::WAVE_05.EPL` | EPL | 14752 | `362d3e3b9d86e96cc408b2ac392838551848ecb27d829fe2996d1581b7132cfc` |
| `data_de.cpk::event/fishing.bin::WAVE_06.EPL` | EPL | 15232 | `7f2fc8f2c4b6baf6b48cecabdfb1ab1c09b3971cab53ea9933f4df36846d71f9` |
| `data_de.cpk::event_data/community.bin::CARD_30.EPL` | EPL | 267120 | `51a878d3a3d380b26abeab633ad5e075894c519e3fa3436cce36af0d81cc6c91` |
| `data_de.cpk::event_data/community.bin::CARD_EF04.EPL` | EPL | 153600 | `feb474459c1fb4b4d3b11ea541d82ea2673a32a14118275c43e0430a21ec9311` |
| `data_de.cpk::event_data/community.bin::CARD_EF06.EPL` | EPL | 134112 | `cb1ed7c4957fdbd26b0081735cb9b4c0c55587eeb604c5d868c972e738011621` |
| `data_de.cpk::event_data/community.bin::CARD_EF07.EPL` | EPL | 19440 | `2b55441af548b5b3c90bc1bad8253942d9f68fe7cfa2cdf75779d5f0dcbc86f4` |
| `data_de.cpk::event_data/community.bin::CMY01_02.EPL` | EPL | 7184 | `4d4eee955d10ab94f0672798d119f60466cb4437c3027c45ad8d13c1cd826b73` |
| `data_de.cpk::event_data/community.bin::CMY02_01.EPL` | EPL | 7232 | `b5561bee072cbe2532eb2829bf5c39fc91927f53fdf184e65a67b8d01afc5dfe` |
| `data_de.cpk::event_data/community.bin::CMY04_06.EPL` | EPL | 2816 | `1d99560e14f4d5304bad68a200e428f909d7b806466b1e504b7c8cb1a5b79800` |
| `data_de.cpk::event_data/community.bin::CMY05_05.EPL` | EPL | 13440 | `4252a6171efe301668d8d50dc48ad1d5157e8fb7ea0e10e1322c31690c4747e1` |
| `data_de.cpk::event_data/community.bin::E_KIRA.EPL` | EPL | 12560 | `9b83128cbed2ba53cb6c6bea178e6a26ba031a32db7e39f0863e09adac8b5e9c` |
| `data_de.cpk::event_data/community.bin::E_KIRA2.EPL` | EPL | 12560 | `cbc82cec6fd117a07c16d78d18265390c36c0ec058022e0f36fba5e944fabf85` |
| `data_de.cpk::event_data/community.bin::P_GET3.EPL` | EPL | 2128 | `928fd9aaecd4c39732a2c2492670dac71de00a6dda594d74c2316286a5127c9d` |
| `data_de.cpk::facility/cmbroot.arc::E_PER.EPL` | EPL | 1120 | `07b9bac782b287964bc963b576ddc16a1724ce44010e3bf9a86f6972a757a1a9` |
| `data_de.cpk::facility/cmbroot.arc::GAT07_01.EPL` | EPL | 19840 | `367b85f1f9785cb0a1aace1573b309b9a9eecf9df2d174eef2d78cdc332c8d16` |
| `data_de.cpk::facility/cmbroot.arc::PER3.EPL` | EPL | 640 | `a8420c4ae86d038a3f45da5fed5afb3bcc040783e1aa5372c1957916fb5bf9b8` |
| `data_de.cpk::field/pack/fd031_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_de.cpk::field/pack/fd031_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_de.cpk::field/pack/fd031_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_de.cpk::field/pack/fd031_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_de.cpk::field/pack/fd069_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_de.cpk::field/pack/fd069_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_de.cpk::field/pack/fd069_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_de.cpk::field/pack/fd069_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_de.cpk::field/pack/fd069_003.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_de.cpk::field/pack/fd069_003.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_de.cpk::field/pack/fd069_004.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_de.cpk::field/pack/fd069_004.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON1.EPL` | EPL | 35360 | `419957b5504a91136e2a6da0a9191adbb0d0fdcabec7745d7c6a0245337e1a0c` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON10.EPL` | EPL | 640 | `a459f3d6a49e217485c18351e1be0074790389f619fa11a21c41d10f817a629e` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON11.EPL` | EPL | 35360 | `a2b85ec8fd28435027137c00eb3db2613488890e426eb15950693e450ae3666e` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON12.EPL` | EPL | 13920 | `7c600dbf814ca52978843c10998ac5877290426bce5fc15b11d188ced6dc7655` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON13.EPL` | EPL | 2816 | `4c470d96fa640beb21f4b5c5f69da7278816df340150539b19a6d282b58d3745` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON14.EPL` | EPL | 3200 | `44511407c6c60dcb3fdffc2ea0b295f3e367373ad4e47fafb8f68ffbfa523742` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON15.EPL` | EPL | 3776 | `f376c7bd0d6fd7aa6614093b86b47465f5ce418e4456293015687a845851c6c0` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON16.EPL` | EPL | 8960 | `749535ddb03ac80e9c173a4e2c4e011bb445ef4c3afea446d9327356e03e53e8` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON17.EPL` | EPL | 5408 | `682ce6151e173a4e5d837be7324c60c7e5475112682d4f7734f32c2200316fa1` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON18.EPL` | EPL | 2816 | `ac6cae65586ea536872e38741e8d1b4f710faaa61209c23a54c2daf629f38528` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON19.EPL` | EPL | 10784 | `1712a4d006ad15b02ac959902c2ea753f852832ec6d94c459ca0dd92b2f31ed1` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON2.EPL` | EPL | 70560 | `d1f15c72717d7ae5b2b5b495c209f9ce6928d98bdd7a0a9e0d45e6c95ba8e12b` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON20.EPL` | EPL | 11456 | `3c36b0b747116189a73e67773c05f474611512d39db9f7a81c47ce562f40632e` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON21.EPL` | EPL | 16768 | `ca32d5e7033db22860116d6accd7983ace3ee5c840f8ef936f22566b987bca92` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON22.EPL` | EPL | 4448 | `d0d9ad27c09e15df5caccb17467b71e5b15ba39d20f2c325d4d445caa7aac979` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON23.EPL` | EPL | 7328 | `83a7d4ae49cc4d9237798ed7eb905ef31e237550537bd89f67cf5744fbf8d102` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON24.EPL` | EPL | 13440 | `91538d6b1c7770ad8a4b4a2c34556939c9c1e4f1fc1beede20f8ba87ac5d5e7a` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON25.EPL` | EPL | 52960 | `5d8963dec880d512cc7453b46e2c0187e4113a21f82a431656472a48467951e5` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON26.EPL` | EPL | 2816 | `37621fb4cae82626436d81fabdc9b4177905063ac5264703e7de25f58e06bad3` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON3.EPL` | EPL | 12928 | `c8174ba08c05c1d7c62cb57138c3487e376dcf16713d3341a6018fc413e46688` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON4.EPL` | EPL | 32928 | `2f173d83c1e9a766c6bd5f21b90812ba376f44e2c7fd4b314b3e8c3004087b64` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON5.EPL` | EPL | 10464 | `72c96b08b6fdf5fcc5aad1a3dc4961b30e063647d1c8bac2a51ec464789c7bdd` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON6.EPL` | EPL | 24064 | `9115e4344e6efa200a9b2bad425236d6c514a6f4109e1154b080140c0889cd1b` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON7.EPL` | EPL | 2816 | `dedee54a4f210c6a3e993d17ed78bbc896a6c1e97acafc1e50e21b7ceb78f149` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON8.EPL` | EPL | 5472 | `15038b74e6204b01c8f1fee6595dba30b926e4a37e052bc8de8f53b10fb77dec` |
| `data_de.cpk::init.bin::event/icon.bin::_ICON9.EPL` | EPL | 34688 | `f4970730444758d9b1e2c20933ecdba4d9c69a92422d4722eb33caf384c0491b` |
| `data_de.cpk::init.bin::field/F_BLUR.EPL` | EPL | 640 | `f7c0c9b76c0cc4cb11dbe0816d1d3d83ef72ccd00f33a9480acf0ecd55fcfb6c` |
| `data_de.cpk::init.bin::field/F_ICON01.EPL` | EPL | 11856 | `bb0d1adabff9e8808efb526a1d5ed9a438e00209958956d7fd8b83d811836882` |
| `data_de.cpk::init.bin::field/F_ICON02.EPL` | EPL | 11312 | `37486846f11641eae744873620fd47689a2b343adebd53a567287c1918007555` |
| `data_de.cpk::init.bin::field/F_ICON03.EPL` | EPL | 11952 | `9f58f431f3147529befe48618a378f464538847caf9d1ac7ec6cf0fb2a1c21c7` |
| `data_de.cpk::init.bin::field/F_ICON04.EPL` | EPL | 11312 | `a2b0a94439e8716ecece6426d24874563ad7d39294abd923b21c226ca890eeb7` |
| `data_de.cpk::init.bin::field/F_ICON05.EPL` | EPL | 11312 | `24aa952f33209549dd9a2c0ed01f2bf1f9e1f3d1883fcd1bdcdca959d4c827ca` |
| `data_de.cpk::init.bin::field/RAIN_01.EPL` | EPL | 9440 | `af7e203dbc36d37d53577a673f20b9a6b71fa5bd1b52189326d4ea476e560c5e` |
| `data_de.cpk::init.bin::field/SNOW_01.EPL` | EPL | 640 | `4c614a3d103ad5745d5610865f9e2cdac2fc4b90c04b343d1f3ff61401c0bba1` |
| `data_de.cpk::init.bin::field/SNOW_02.EPL` | EPL | 640 | `3f025061842c24fa748cdd09adabd7de493c95eae332e544557efd0602798454` |
| `data_de.cpk::init.bin::field/effect/BREATH_01.EPL` | EPL | 640 | `9e5da14b05e6803b490c3f6e71ceac80965aaba7184b3ce317229ecf53617630` |
| `data_de.cpk::init.bin::field/effect/FOOT_L.EPL` | EPL | 5728 | `e4169139debccf8f28e88e86e2b72a49e10ecfee15a61eb745fd1a1bf1f18eee` |
| `data_de.cpk::init.bin::field/effect/FOOT_R.EPL` | EPL | 5728 | `83828867762a5b9ed87f11eb45f91284974350052e85d8993a737fc0229e12a2` |
| `data_de.cpk::init.bin::insect/insect.bin::INSECT_01.EPL` | EPL | 9696 | `74be2671465a4e5e760c6697cd27f5e96dce151f48b488fc40262c75cda40ea7` |
| `data_de.cpk::init_free.bin::battle/effect/SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::ATC_D_J.BED` | BED400 | 4800 | `b4265885008f1b5c154dbd746eac4e9a94b2a4237eac93782fbe6e07626064dc` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::ATC_J_J.BED` | BED400 | 4192 | `62a5fe27dc681c5792c9018cf527d7156942414a374de0668ce15bb4802dec85` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 4576 | `0fa03eedfe3a6eca662708990748cf9b8332770038d9a957a48d27672681ae07` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D_J.BED` | BED400 | 4800 | `b4265885008f1b5c154dbd746eac4e9a94b2a4237eac93782fbe6e07626064dc` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J_J.BED` | BED400 | 4192 | `62a5fe27dc681c5792c9018cf527d7156942414a374de0668ce15bb4802dec85` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 4576 | `0fa03eedfe3a6eca662708990748cf9b8332770038d9a957a48d27672681ae07` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_e.cpk::battle/boss/e200.bin::BED/e200_ENCOUNT.BED` | BED400 | 14288 | `4294f933ecf65bfba4e1b444999c903c3c47a29462602e79b53ed7a2e7ceb452` |
| `data_e.cpk::battle/boss/e200.bin::BED/e200_END.BED` | BED400 | 33088 | `931f2c31d66a9853504d6ad85b72593d9e62da352f97719a930c8bed0e3ab733` |
| `data_e.cpk::battle/boss/e200.bin::BED/e200_H160.BED` | BED400 | 9584 | `1a0d373c01cb5744cd98f6268da56e265539fe9b10a18a20d880348c77bc727a` |
| `data_e.cpk::battle/boss/e200.bin::BED/e200_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_e.cpk::battle/boss/e200.bin::BED/e200_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e200.bin::BED/e200_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e200.bin::BED/e200_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e200.bin::BED/e200_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e201.bin::BED/e201_ENCOUNT.BED` | BED400 | 10080 | `cf6ee31d878f652f19a197d9b18502f5e32e4c765131066088aa7a008feda7bb` |
| `data_e.cpk::battle/boss/e201.bin::BED/e201_END.BED` | BED400 | 30192 | `b31d78e5de0092924773b1d1b2e55a1955a77c1e2175c64ea794dba20d1c32fd` |
| `data_e.cpk::battle/boss/e201.bin::BED/e201_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_e.cpk::battle/boss/e201.bin::BED/e201_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e201.bin::BED/e201_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e201.bin::BED/e201_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e201.bin::BED/e201_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e202.bin::BED/e202_ENCOUNT.BED` | BED400 | 7344 | `5802ab6fc50884bbc4e1db4f97b92b60079582f1126c2ad60cc8e44567c4fc8c` |
| `data_e.cpk::battle/boss/e202.bin::BED/e202_END.BED` | BED400 | 30992 | `28078663ea649edae5bcd52b50f6e614bfb54f41f722d5d18654ea082fe50bc6` |
| `data_e.cpk::battle/boss/e202.bin::BED/e202_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_e.cpk::battle/boss/e202.bin::BED/e202_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e202.bin::BED/e202_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e202.bin::BED/e202_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e202.bin::BED/e202_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e203.bin::BED/e203_ENCOUNT.BED` | BED400 | 8960 | `97081976c63beb477742d9e10a2a83bf02c08fbac7c5e5a0c727b332304cf803` |
| `data_e.cpk::battle/boss/e203.bin::BED/e203_END.BED` | BED400 | 35536 | `6221812dc3626928a9ab734835fce955f56d3164995195624b7d20907e249863` |
| `data_e.cpk::battle/boss/e203.bin::BED/e203_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_e.cpk::battle/boss/e203.bin::BED/e203_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e203.bin::BED/e203_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e203.bin::BED/e203_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e203.bin::BED/e203_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_e.cpk::battle/boss/e203.bin::BED/e203_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e204.bin::BED/e204_ENCOUNT.BED` | BED400 | 9152 | `2b707e32159c57f61dda2a51f9a4d1bb13dbaff059eb0b294294771e29b2c155` |
| `data_e.cpk::battle/boss/e204.bin::BED/e204_END.BED` | BED400 | 7040 | `ff3be583ba835a33b94a87a47080141ccda1aaa9cbe5946ce80a861386da0534` |
| `data_e.cpk::battle/boss/e204.bin::BED/e204_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_e.cpk::battle/boss/e204.bin::BED/e204_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e204.bin::BED/e204_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e204.bin::BED/e204_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e204.bin::BED/e204_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_e.cpk::battle/boss/e204.bin::BED/e204_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e205.bin::BED/e205_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e205.bin::BED/e205_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e205.bin::BED/e205_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e205.bin::BED/e205_ENCOUNT.BED` | BED400 | 10336 | `2f5fad7c4101340a9f82380c62dc0341cf2218734ebcfb157e4f68be0eca017b` |
| `data_e.cpk::battle/boss/e205.bin::BED/e205_END.BED` | BED400 | 32320 | `151da6e1d4d39174971a53d45f228d2ef3e60e9513815541c9b7dc0e03187fe1` |
| `data_e.cpk::battle/boss/e205.bin::BED/e205_HATUDO.BED` | BED400 | 3696 | `18feb0b9a37f5d925e6a17a49030adb5b2280a2cb28fb6261a6dd1460507cdb0` |
| `data_e.cpk::battle/boss/e205.bin::BED/e205_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e205.bin::BED/e205_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e205.bin::BED/e205_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e205.bin::BED/e205_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_e.cpk::battle/boss/e205.bin::BED/e205_KAIJYO.BED` | BED400 | 5152 | `99bfb77d8d42d84c48c89e01526da14dd68075d99521a820c0675e0450e1d373` |
| `data_e.cpk::battle/boss/e205.bin::BED/e205_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e206.bin::BED/e206_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e206.bin::BED/e206_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e206.bin::BED/e206_ENCOUNT.BED` | BED400 | 6400 | `5b9ca9679564c387f222fcd3aefbba7ca6b0719893a33f2f14f35e1d09b064ce` |
| `data_e.cpk::battle/boss/e206.bin::BED/e206_END.BED` | BED400 | 31504 | `ee15eb497f11a707b85ad4c857527ba2732eaf4f9936fce305bbb4056de1d85d` |
| `data_e.cpk::battle/boss/e206.bin::BED/e206_H172.BED` | BED400 | 7552 | `74a7944243d6c20da666245d7ba3d0e76b957cb06136a5de79bbf03cd9c5f1bb` |
| `data_e.cpk::battle/boss/e206.bin::BED/e206_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_e.cpk::battle/boss/e206.bin::BED/e206_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e206.bin::BED/e206_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e206.bin::BED/e206_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e206.bin::BED/e206_H_SP.BED` | BED400 | 4064 | `7007fbcdd175abbf881d3ae61fe12bed93872c08931f3e6b82aed5bfc83a2dc7` |
| `data_e.cpk::battle/boss/e206.bin::BED/e206_OFF.BED` | BED400 | 8208 | `3a6e572de45506664a54fa77e2f355094be47cc77a6b603306bad35f8b940175` |
| `data_e.cpk::battle/boss/e206.bin::BED/e206_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e207.bin::BED/e207_ENCOUNT.BED` | BED400 | 6944 | `e5db68919f29386aa0f2142e7da82e62bea35af0985694fb2a2ded112255088f` |
| `data_e.cpk::battle/boss/e207.bin::BED/e207_END.BED` | BED400 | 32096 | `cc1f9d2b5dec6f8e5b3f3fa82b5710399537f04c31bb09896f4cee7a11e8d383` |
| `data_e.cpk::battle/boss/e207.bin::BED/e207_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_e.cpk::battle/boss/e207.bin::BED/e207_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e207.bin::BED/e207_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e207.bin::BED/e207_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e207.bin::BED/e207_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e208.bin::BED/e208_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e208.bin::BED/e208_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e208.bin::BED/e208_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e208.bin::BED/e208_ENCOUNT.BED` | BED400 | 14032 | `2275fd23ee2bea212640b51f317d363e17a98e93db834cba7a104090bb652b60` |
| `data_e.cpk::battle/boss/e208.bin::BED/e208_END.BED` | BED400 | 32336 | `b4a227dc5312f1dc2e4df9c6c0e5c11216ee2c24a029b1b7a209b50757dd2bde` |
| `data_e.cpk::battle/boss/e208.bin::BED/e208_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_e.cpk::battle/boss/e208.bin::BED/e208_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e208.bin::BED/e208_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e208.bin::BED/e208_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e208.bin::BED/e208_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_e.cpk::battle/boss/e208.bin::BED/e208_SK17B.BED` | BED400 | 13328 | `0d0a8231a695a1e9497d98246dfc0c544a03b95f827f3a45adb2fd3ffeb7a704` |
| `data_e.cpk::battle/boss/e208.bin::BED/e208_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e209.bin::BED/e209_ENCOUNT.BED` | BED400 | 9392 | `80719c0c13a8fde7ff0e545957665da6cffd72631c30e8525e0962927f919266` |
| `data_e.cpk::battle/boss/e209.bin::BED/e209_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data_e.cpk::battle/boss/e209.bin::BED/e209_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_e.cpk::battle/boss/e209.bin::BED/e209_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_e.cpk::battle/boss/e209.bin::BED/e209_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_e.cpk::battle/boss/e209.bin::BED/e209_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_e.cpk::battle/boss/e209.bin::BED/e209_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20A.bin::BED/e20A_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20A.bin::BED/e20A_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20A.bin::BED/e20A_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20A.bin::BED/e20A_ENCOUNT.BED` | BED400 | 7248 | `7ce81d540666b51712f628a3ace38d282202ae8035cd6c497bcba877582a0d26` |
| `data_e.cpk::battle/boss/e20A.bin::BED/e20A_END.BED` | BED400 | 30736 | `0a7fd4451e4224bbd65059a54872ed290c3a637eff66b564a19a7cafb69e82ec` |
| `data_e.cpk::battle/boss/e20A.bin::BED/e20A_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_e.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20A.bin::BED/e20A_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_e.cpk::battle/boss/e20A.bin::BED/e20A_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20B.bin::BED/e20B_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20B.bin::BED/e20B_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20B.bin::BED/e20B_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT.BED` | BED400 | 7408 | `bfbcd65ffdff97966112ba29739ca11224d5130bd9e24793232706a5d78b80c3` |
| `data_e.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT2.BED` | BED400 | 6976 | `f14a2a742989b86d727615b805d73d35b92cfa2e98e4f1f2715b33c7850086aa` |
| `data_e.cpk::battle/boss/e20B.bin::BED/e20B_END.BED` | BED400 | 20400 | `fd46f5973ff92fc5d781a282fb094494df3e8a55809730bd91d623b8b55180d8` |
| `data_e.cpk::battle/boss/e20B.bin::BED/e20B_END1.BED` | BED400 | 52608 | `8069c2bcc904b6fa9ed727768508d39a56ecc95f3ce509fbb4d92d5c9c1073fb` |
| `data_e.cpk::battle/boss/e20B.bin::BED/e20B_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_e.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20B.bin::BED/e20B_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_e.cpk::battle/boss/e20B.bin::BED/e20B_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20C.bin::BED/e20C_ENCOUNT.BED` | BED400 | 8016 | `0d7531f757e8a743070f66980adba7097a34223c7ceab5b7e15e1006db2b0f76` |
| `data_e.cpk::battle/boss/e20C.bin::BED/e20C_END.BED` | BED400 | 23760 | `98e314cd795aabd6134413891b58afcd5b06a5fa4c2668ab5014bb7c9e19ec9f` |
| `data_e.cpk::battle/boss/e20C.bin::BED/e20C_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_e.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20C.bin::BED/e20C_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e20D.bin::BED/e20D_ENCOUNT.BED` | BED400 | 6592 | `d23955927d2d12aa710e56458ce8d64c9b172cedcea6c36cd99c5696fb1d049f` |
| `data_e.cpk::battle/boss/e20D.bin::BED/e20D_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data_e.cpk::battle/boss/e20D.bin::BED/e20D_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_e.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_e.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_e.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_e.cpk::battle/boss/e20D.bin::BED/e20D_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e214.bin::BED/e214_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e214.bin::BED/e214_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e214.bin::BED/e214_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e214.bin::BED/e214_ENCOUNT.BED` | BED400 | 10176 | `b56f07d78e20e9c4dbd1adc3f9c582035dd213c4a751d4a6df5889ddd7492a3d` |
| `data_e.cpk::battle/boss/e214.bin::BED/e214_END.BED` | BED400 | 5696 | `d109a2a14e0ee8b4fd4764e6c08aa8b78cb68e76651a42ac48c3638a0361576a` |
| `data_e.cpk::battle/boss/e214.bin::BED/e214_HATUDO.BED` | BED400 | 3504 | `b1792d9bd7867be710d47f27d81220d79495ec12d027191ddcd917f963e0ead6` |
| `data_e.cpk::battle/boss/e214.bin::BED/e214_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e214.bin::BED/e214_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e214.bin::BED/e214_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e214.bin::BED/e214_H_SP.BED` | BED400 | 3696 | `122de1e6d1504bf72f5f0fb267dc05aa3e80483c32cf8db7e8330feb9cfcce82` |
| `data_e.cpk::battle/boss/e214.bin::BED/e214_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e215.bin::BED/e215_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e215.bin::BED/e215_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e215.bin::BED/e215_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e215.bin::BED/e215_ENCOUNT.BED` | BED400 | 11296 | `e99453fa2913f5bbcad4d7c7b3ac29df97d5d35f5c84188af83b20bc5f4b35cf` |
| `data_e.cpk::battle/boss/e215.bin::BED/e215_END.BED` | BED400 | 1230864 | `c30afdd4e4cf82c8eebe4aa26faf5893a3427e5bd1595dd635f9aa63e89c9fc5` |
| `data_e.cpk::battle/boss/e215.bin::BED/e215_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_e.cpk::battle/boss/e215.bin::BED/e215_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e215.bin::BED/e215_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e215.bin::BED/e215_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e215.bin::BED/e215_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_e.cpk::battle/boss/e215.bin::BED/e215_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e216.bin::BED/e216_ENCOUNT.BED` | BED400 | 11200 | `1dcc41a7929352eb50ba07e44f68d24ed1c0a12e8cbe954e7cb75f735f1da34e` |
| `data_e.cpk::battle/boss/e216.bin::BED/e216_END.BED` | BED400 | 10576 | `49b1c46532ed9d3cc5a87e47736efd0ea4ec0584910f0d09d6b57ad1153d3292` |
| `data_e.cpk::battle/boss/e216.bin::BED/e216_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_e.cpk::battle/boss/e216.bin::BED/e216_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e216.bin::BED/e216_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e216.bin::BED/e216_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e216.bin::BED/e216_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_e.cpk::battle/boss/e216.bin::BED/e216_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/boss/e217.bin::BED/e217_ENCOUNT.BED` | BED400 | 6304 | `959dca87cf9853542ee07ada31ffde9a842967b3194b9a38fec7e0e912fac55c` |
| `data_e.cpk::battle/boss/e217.bin::BED/e217_END.BED` | BED400 | 60592 | `ea818bdc095b1797f4927266ae081698520a0b3e4b3f4c4d33693bfd57d28f82` |
| `data_e.cpk::battle/boss/e217.bin::BED/e217_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_e.cpk::battle/boss/e217.bin::BED/e217_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_e.cpk::battle/boss/e217.bin::BED/e217_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_e.cpk::battle/boss/e217.bin::BED/e217_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_e.cpk::battle/boss/e217.bin::BED/e217_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_e.cpk::battle/btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_e.cpk::battle/btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_e.cpk::battle/btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_e.cpk::battle/btleff.bin::ATC_D_J.BED` | BED400 | 4800 | `b4265885008f1b5c154dbd746eac4e9a94b2a4237eac93782fbe6e07626064dc` |
| `data_e.cpk::battle/btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_e.cpk::battle/btleff.bin::ATC_J_J.BED` | BED400 | 4192 | `62a5fe27dc681c5792c9018cf527d7156942414a374de0668ce15bb4802dec85` |
| `data_e.cpk::battle/btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_e.cpk::battle/btleff.bin::ATC_Z_J.BED` | BED400 | 4576 | `0fa03eedfe3a6eca662708990748cf9b8332770038d9a957a48d27672681ae07` |
| `data_e.cpk::battle/btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_e.cpk::battle/btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_e.cpk::battle/btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_e.cpk::battle/btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_e.cpk::battle/btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_e.cpk::battle/btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_e.cpk::battle/btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_e.cpk::battle/btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_e.cpk::battle/btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_e.cpk::battle/btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_e.cpk::battle/btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_e.cpk::battle/btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_e.cpk::battle/btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_e.cpk::battle/btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_e.cpk::battle/btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_e.cpk::battle/btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_e.cpk::battle/btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_e.cpk::battle/btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_e.cpk::battle/btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_e.cpk::battle/btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_e.cpk::battle/btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_e.cpk::battle/btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_e.cpk::battle/btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_e.cpk::battle/btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_e.cpk::battle/btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_e.cpk::battle/btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_e.cpk::battle/btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_e.cpk::battle/btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_e.cpk::battle/btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_e.cpk::battle/btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_e.cpk::battle/btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_e.cpk::battle/btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_e.cpk::battle/btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_e.cpk::battle/btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_e.cpk::battle/btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_e.cpk::battle/btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_e.cpk::battle/btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_e.cpk::battle/btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_e.cpk::battle/btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_e.cpk::battle/btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_e.cpk::battle/btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_e.cpk::battle/cutin/syoukan/SYKN_1.EPL` | EPL | 144336 | `26434f8e512a635e6015ad68637633ea8d0cc4572e822b46e5003ee5c1333cf3` |
| `data_e.cpk::battle/cutin/syoukan/SYKN_2.EPL` | EPL | 144432 | `ab3c4799776a43b07bf9dbb4ae839f87c5d2ace9270c419f9793dfd690f409e0` |
| `data_e.cpk::battle/cutin/syoukan/SYKN_3.EPL` | EPL | 144624 | `86ee77c0834af7bfd0ba3083fe75c6e5767ca5d52b0147deaa1545ff69f6eae1` |
| `data_e.cpk::battle/cutin/syoukan/SYKN_4.EPL` | EPL | 144848 | `27e4d5fa9e14b7a70a039a8661c4f478617fc51663f9a4261d0cafaf6908a963` |
| `data_e.cpk::battle/cutin/syoukan/SYKN_6.EPL` | EPL | 144400 | `de11427ae9239136204dcbc025a1396ffd025875a47bfde48b1cc8591c3bd4d9` |
| `data_e.cpk::battle/cutin/syoukan/SYKN_7.EPL` | EPL | 143472 | `617e49da072f4b562784ed367731fa98839e7ff531c0509924fb71a09fc331fd` |
| `data_e.cpk::battle/cutin/syoukan/SYKN_8.EPL` | EPL | 140368 | `ebaa26e8de1adee3664d90898b5b9d34809560757b64d372024b6f3ddef432ff` |
| `data_e.cpk::battle/cutin/syoukan/SYKN_E109.EPL` | EPL | 173104 | `f121cad23dcd1d39b2fe45b4aa37732a94512ef064b4f985d34a726818aa7350` |
| `data_e.cpk::battle/effect/ATC_D_J.BED` | BED400 | 4784 | `b2a997bc6b785f052ecb4b46525dc7fe46e8cf95ac1cd96f8ef6be70b183be3a` |
| `data_e.cpk::battle/effect/ATC_J_J.BED` | BED400 | 4176 | `7885eb1b721aa378841a0263edb4f37da292b661dff468638220250f786b02cc` |
| `data_e.cpk::battle/effect/ATC_Z_J.BED` | BED400 | 4560 | `fd20d2dfd4ef54959794e760a3ed637fbfbcfa4da07521399c18712b9fa78da6` |
| `data_e.cpk::battle/result/shuffle.arc::SHUFFLE01.EPL` | EPL | 6368 | `bb7e2d674c4b18778f6f7b02304159ee7a192b90f38406353bd32754207244b5` |
| `data_e.cpk::battle/result/shuffle.arc::SHUFF_01.EPL` | EPL | 8864 | `59627e77df24e98b1aec5cf22a9c47495e145505c6fea06b5b0429b3427a7fad` |
| `data_e.cpk::battle/result/shuffle.arc::SHUFF_02.EPL` | EPL | 9760 | `58c0739b468638264c6d3534bd2b2c6f5af07a3e6d878fc4104351a4f01b9dad` |
| `data_e.cpk::battle/result/shuffle.arc::SHUFF_03.EPL` | EPL | 14688 | `c50a3a6c14cb67013b24aa7f12833c401ef5ef7f5bb823ec4a45916b1d996782` |
| `data_e.cpk::battle/result/shuffle.arc::SHUFF_04.EPL` | EPL | 9568 | `2ac489ffa035960b485696279cdcdc702e512321dfec3c5de3e0533354ac27ef` |
| `data_e.cpk::battle/result/shuffle.arc::SHUFF_05.EPL` | EPL | 10144 | `1c02f0750e4a6e99932d04af21a9752ae50b80597201fbc7ce944bdde2b45432` |
| `data_e.cpk::battle/result/shuffle.arc::SHUFF_06.EPL` | EPL | 15104 | `a7756c94b0471c8bedc1447c499df3233be21f8992655205858fb1e97e7d7c96` |
| `data_e.cpk::battle/result/shuffle.arc::SHUFF_09.EPL` | EPL | 1600 | `7db6177b81d30997b063517caf3b0a1a1781a7363f3219940aaa1d3deef01bb4` |
| `data_e.cpk::battle/union/U15D_A.BED` | BED400 | 229136 | `bf70bea6d255daeff4ef444131c15e45091c85bc4e6344ff1620d298283d10cc` |
| `data_e.cpk::battle/union/U15E_A.BED` | BED400 | 262672 | `b1cb3137884d0155a5b2b10fbf2fa5c0a79c808cb9499c24c862986e16fba59b` |
| `data_e.cpk::battle/union/U15F_A.BED` | BED400 | 251840 | `bd8a7a2b11b045eb1c37fe24bdb438a382444e9521e3726c09952b7121c4bf94` |
| `data_e.cpk::event/fishing.bin::FLOAT_01.EPL` | EPL | 7232 | `3f7721ea180386dc79379572ea567e7521b2a470d9f76167aa71bbdebdb3f5d0` |
| `data_e.cpk::event/fishing.bin::FLOAT_WV.EPL` | EPL | 7264 | `db0358e93caaab0ccfe666c308ece5ff9acc4522ae95044fbc00be249dadb346` |
| `data_e.cpk::event/fishing.bin::SPRAY_01.EPL` | EPL | 11424 | `6229b4e3eae59c7f62ee785737578b3aba127205f979dd528d31222cb60d0be3` |
| `data_e.cpk::event/fishing.bin::SPRAY_02.EPL` | EPL | 4288 | `54cabd45d19febb6d5decaeb2ee9c081f8c34a1e3273c2db20449cc0fb637aa1` |
| `data_e.cpk::event/fishing.bin::SPRAY_03.EPL` | EPL | 23296 | `34836ad2132b20984fab062cf62efbae171cd5438d7d64d11d3dbe06661d15f3` |
| `data_e.cpk::event/fishing.bin::SPRAY_04.EPL` | EPL | 22880 | `9d383ff669c401e9ff458d0904761040d95cfbd90b64b6aef2bf04dded321238` |
| `data_e.cpk::event/fishing.bin::WAVE_01.EPL` | EPL | 512 | `e73f641168a4d70f2cfcb10cf9d4bbb71fcf1466208de78c682d22e8ba876bb3` |
| `data_e.cpk::event/fishing.bin::WAVE_02.EPL` | EPL | 512 | `e623b56cce128bbc917f276a6238991b9417971c74de892194acc4e17610a982` |
| `data_e.cpk::event/fishing.bin::WAVE_03.EPL` | EPL | 864 | `7431335642b1c77dc423964534743a10b1c9193279f2e36078cba7315e7a8fd4` |
| `data_e.cpk::event/fishing.bin::WAVE_04.EPL` | EPL | 82368 | `5975b6978e545c03916310565458748458e384874ecd227887f43dc0eb331c5f` |
| `data_e.cpk::event/fishing.bin::WAVE_05.EPL` | EPL | 14752 | `362d3e3b9d86e96cc408b2ac392838551848ecb27d829fe2996d1581b7132cfc` |
| `data_e.cpk::event/fishing.bin::WAVE_06.EPL` | EPL | 15232 | `7f2fc8f2c4b6baf6b48cecabdfb1ab1c09b3971cab53ea9933f4df36846d71f9` |
| `data_e.cpk::event_data/community.bin::CARD_30.EPL` | EPL | 267120 | `51a878d3a3d380b26abeab633ad5e075894c519e3fa3436cce36af0d81cc6c91` |
| `data_e.cpk::event_data/community.bin::CARD_EF04.EPL` | EPL | 153600 | `feb474459c1fb4b4d3b11ea541d82ea2673a32a14118275c43e0430a21ec9311` |
| `data_e.cpk::event_data/community.bin::CARD_EF06.EPL` | EPL | 134112 | `cb1ed7c4957fdbd26b0081735cb9b4c0c55587eeb604c5d868c972e738011621` |
| `data_e.cpk::event_data/community.bin::CARD_EF07.EPL` | EPL | 19440 | `2b55441af548b5b3c90bc1bad8253942d9f68fe7cfa2cdf75779d5f0dcbc86f4` |
| `data_e.cpk::event_data/community.bin::CMY01_02.EPL` | EPL | 7184 | `4d4eee955d10ab94f0672798d119f60466cb4437c3027c45ad8d13c1cd826b73` |
| `data_e.cpk::event_data/community.bin::CMY02_01.EPL` | EPL | 7232 | `b5561bee072cbe2532eb2829bf5c39fc91927f53fdf184e65a67b8d01afc5dfe` |
| `data_e.cpk::event_data/community.bin::CMY04_06.EPL` | EPL | 2816 | `1d99560e14f4d5304bad68a200e428f909d7b806466b1e504b7c8cb1a5b79800` |
| `data_e.cpk::event_data/community.bin::CMY05_05.EPL` | EPL | 13440 | `4252a6171efe301668d8d50dc48ad1d5157e8fb7ea0e10e1322c31690c4747e1` |
| `data_e.cpk::event_data/community.bin::E_KIRA.EPL` | EPL | 12560 | `9b83128cbed2ba53cb6c6bea178e6a26ba031a32db7e39f0863e09adac8b5e9c` |
| `data_e.cpk::event_data/community.bin::E_KIRA2.EPL` | EPL | 12560 | `cbc82cec6fd117a07c16d78d18265390c36c0ec058022e0f36fba5e944fabf85` |
| `data_e.cpk::event_data/community.bin::P_GET3.EPL` | EPL | 2128 | `928fd9aaecd4c39732a2c2492670dac71de00a6dda594d74c2316286a5127c9d` |
| `data_e.cpk::facility/cmbroot.arc::E_PER.EPL` | EPL | 1120 | `07b9bac782b287964bc963b576ddc16a1724ce44010e3bf9a86f6972a757a1a9` |
| `data_e.cpk::facility/cmbroot.arc::GAT07_01.EPL` | EPL | 19840 | `367b85f1f9785cb0a1aace1573b309b9a9eecf9df2d174eef2d78cdc332c8d16` |
| `data_e.cpk::facility/cmbroot.arc::PER3.EPL` | EPL | 640 | `a8420c4ae86d038a3f45da5fed5afb3bcc040783e1aa5372c1957916fb5bf9b8` |
| `data_e.cpk::field/SNOW_old.EPL` | EPL | 640 | `ff17b8ecac6518601e522bbc9eaf963fe6b3f480e82167891c228fcf9f8a2a99` |
| `data_e.cpk::field/pack/fd031_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_e.cpk::field/pack/fd031_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_e.cpk::field/pack/fd031_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_e.cpk::field/pack/fd031_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_e.cpk::field/pack/fd069_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_e.cpk::field/pack/fd069_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_e.cpk::field/pack/fd069_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_e.cpk::field/pack/fd069_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_e.cpk::field/pack/fd069_003.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_e.cpk::field/pack/fd069_003.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_e.cpk::field/pack/fd069_004.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_e.cpk::field/pack/fd069_004.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON1.EPL` | EPL | 35360 | `419957b5504a91136e2a6da0a9191adbb0d0fdcabec7745d7c6a0245337e1a0c` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON10.EPL` | EPL | 640 | `a459f3d6a49e217485c18351e1be0074790389f619fa11a21c41d10f817a629e` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON11.EPL` | EPL | 35360 | `a2b85ec8fd28435027137c00eb3db2613488890e426eb15950693e450ae3666e` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON12.EPL` | EPL | 13920 | `7c600dbf814ca52978843c10998ac5877290426bce5fc15b11d188ced6dc7655` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON13.EPL` | EPL | 2816 | `4c470d96fa640beb21f4b5c5f69da7278816df340150539b19a6d282b58d3745` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON14.EPL` | EPL | 3200 | `44511407c6c60dcb3fdffc2ea0b295f3e367373ad4e47fafb8f68ffbfa523742` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON15.EPL` | EPL | 3776 | `f376c7bd0d6fd7aa6614093b86b47465f5ce418e4456293015687a845851c6c0` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON16.EPL` | EPL | 8960 | `749535ddb03ac80e9c173a4e2c4e011bb445ef4c3afea446d9327356e03e53e8` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON17.EPL` | EPL | 5408 | `682ce6151e173a4e5d837be7324c60c7e5475112682d4f7734f32c2200316fa1` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON18.EPL` | EPL | 2816 | `ac6cae65586ea536872e38741e8d1b4f710faaa61209c23a54c2daf629f38528` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON19.EPL` | EPL | 10784 | `1712a4d006ad15b02ac959902c2ea753f852832ec6d94c459ca0dd92b2f31ed1` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON2.EPL` | EPL | 70560 | `d1f15c72717d7ae5b2b5b495c209f9ce6928d98bdd7a0a9e0d45e6c95ba8e12b` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON20.EPL` | EPL | 11456 | `3c36b0b747116189a73e67773c05f474611512d39db9f7a81c47ce562f40632e` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON21.EPL` | EPL | 16768 | `ca32d5e7033db22860116d6accd7983ace3ee5c840f8ef936f22566b987bca92` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON22.EPL` | EPL | 4448 | `d0d9ad27c09e15df5caccb17467b71e5b15ba39d20f2c325d4d445caa7aac979` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON23.EPL` | EPL | 7328 | `83a7d4ae49cc4d9237798ed7eb905ef31e237550537bd89f67cf5744fbf8d102` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON24.EPL` | EPL | 13440 | `91538d6b1c7770ad8a4b4a2c34556939c9c1e4f1fc1beede20f8ba87ac5d5e7a` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON25.EPL` | EPL | 52960 | `5d8963dec880d512cc7453b46e2c0187e4113a21f82a431656472a48467951e5` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON26.EPL` | EPL | 2816 | `37621fb4cae82626436d81fabdc9b4177905063ac5264703e7de25f58e06bad3` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON3.EPL` | EPL | 12928 | `c8174ba08c05c1d7c62cb57138c3487e376dcf16713d3341a6018fc413e46688` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON4.EPL` | EPL | 32928 | `2f173d83c1e9a766c6bd5f21b90812ba376f44e2c7fd4b314b3e8c3004087b64` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON5.EPL` | EPL | 10464 | `72c96b08b6fdf5fcc5aad1a3dc4961b30e063647d1c8bac2a51ec464789c7bdd` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON6.EPL` | EPL | 24064 | `9115e4344e6efa200a9b2bad425236d6c514a6f4109e1154b080140c0889cd1b` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON7.EPL` | EPL | 2816 | `dedee54a4f210c6a3e993d17ed78bbc896a6c1e97acafc1e50e21b7ceb78f149` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON8.EPL` | EPL | 5472 | `15038b74e6204b01c8f1fee6595dba30b926e4a37e052bc8de8f53b10fb77dec` |
| `data_e.cpk::init.bin::event/icon.bin::_ICON9.EPL` | EPL | 34688 | `f4970730444758d9b1e2c20933ecdba4d9c69a92422d4722eb33caf384c0491b` |
| `data_e.cpk::init.bin::field/F_BLUR.EPL` | EPL | 640 | `f7c0c9b76c0cc4cb11dbe0816d1d3d83ef72ccd00f33a9480acf0ecd55fcfb6c` |
| `data_e.cpk::init.bin::field/F_ICON01.EPL` | EPL | 11856 | `bb0d1adabff9e8808efb526a1d5ed9a438e00209958956d7fd8b83d811836882` |
| `data_e.cpk::init.bin::field/F_ICON02.EPL` | EPL | 11312 | `37486846f11641eae744873620fd47689a2b343adebd53a567287c1918007555` |
| `data_e.cpk::init.bin::field/F_ICON03.EPL` | EPL | 11952 | `9f58f431f3147529befe48618a378f464538847caf9d1ac7ec6cf0fb2a1c21c7` |
| `data_e.cpk::init.bin::field/F_ICON04.EPL` | EPL | 11312 | `a2b0a94439e8716ecece6426d24874563ad7d39294abd923b21c226ca890eeb7` |
| `data_e.cpk::init.bin::field/F_ICON05.EPL` | EPL | 11312 | `24aa952f33209549dd9a2c0ed01f2bf1f9e1f3d1883fcd1bdcdca959d4c827ca` |
| `data_e.cpk::init.bin::field/RAIN_01.EPL` | EPL | 9440 | `af7e203dbc36d37d53577a673f20b9a6b71fa5bd1b52189326d4ea476e560c5e` |
| `data_e.cpk::init.bin::field/SNOW_01.EPL` | EPL | 640 | `4c614a3d103ad5745d5610865f9e2cdac2fc4b90c04b343d1f3ff61401c0bba1` |
| `data_e.cpk::init.bin::field/SNOW_02.EPL` | EPL | 640 | `3f025061842c24fa748cdd09adabd7de493c95eae332e544557efd0602798454` |
| `data_e.cpk::init.bin::field/effect/BREATH_01.EPL` | EPL | 640 | `9e5da14b05e6803b490c3f6e71ceac80965aaba7184b3ce317229ecf53617630` |
| `data_e.cpk::init.bin::field/effect/FOOT_L.EPL` | EPL | 5728 | `e4169139debccf8f28e88e86e2b72a49e10ecfee15a61eb745fd1a1bf1f18eee` |
| `data_e.cpk::init.bin::field/effect/FOOT_R.EPL` | EPL | 5728 | `83828867762a5b9ed87f11eb45f91284974350052e85d8993a737fc0229e12a2` |
| `data_e.cpk::init.bin::insect/insect.bin::INSECT_01.EPL` | EPL | 9696 | `74be2671465a4e5e760c6697cd27f5e96dce151f48b488fc40262c75cda40ea7` |
| `data_e.cpk::init_free.bin::battle/effect/SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_e.cpk::skill/100.pac::100.BED` | BED400 | 197152 | `2f97867225882794519e02b8922e670d30bb5a4e20767e35c6afc6c549daa68d` |
| `data_e.cpk::skill/101.pac::101.BED` | BED400 | 207296 | `89b3f1aff38916226f5d9e656630471ef4f8ea86a2aee673fad8c4a2e8f65c6f` |
| `data_e.cpk::skill/102.pac::102.BED` | BED400 | 212416 | `363e54d083bddeaa7a29d0f874607db72fd127b75517b9025525c079acbed9b1` |
| `data_e.cpk::skill/11C.pac::11C.BED` | BED400 | 430656 | `9a7b8efb23e3cf010ee3e74002c3a103b99e4457218b54aa882aee7951584f4a` |
| `data_e.cpk::skill/172.pac::172.BED` | BED400 | 109824 | `4e6dbea9b498888bdff410af3fa69cbb08ec3739448505b58ccc11248c09db7b` |
| `data_e.cpk::skill/173.pac::173.BED` | BED400 | 163968 | `ed0a60393c7a6bb50602e0fb756c7fb511e8e4633dfd04f6b5179018edd41a5b` |
| `data_e.cpk::skill/174.pac::174.BED` | BED400 | 248736 | `2e8eb71e41a5bdaebe26af43b1e5d4bec25ee1adc1c98e64404a91ecad6e3ffc` |
| `data_e.cpk::skill/175.pac::175.BED` | BED400 | 248736 | `413a9cd2b9c237c22e2ddfcc993514e558ca7fcfb16667bd9d6d9545769b9710` |
| `data_e.cpk::skill/176.pac::176.BED` | BED400 | 248736 | `140bc518fcbda0494c689ee704fbdc4ec90bff19f17c6b285a91d7b5aa62c5ae` |
| `data_e.cpk::skill/177.pac::177.BED` | BED400 | 248736 | `e37dec52a7ee5c3f831b29e854df61c9a8a68628408478fdb9058b0ae86ab4a2` |
| `data_e.cpk::skill/18E.pac::18E.BED` | BED400 | 212416 | `f6b0c59945969c4c1691867184b5cd7c165f2d41e54e2e1d8692b5efd5c1a595` |
| `data_e.cpk::skill/18F.pac::18F.BED` | BED400 | 407104 | `9543d03537f4d575936424a61dfd9f55d4f83298a9682f08eeb301e408620e0f` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::ATC_D_J.BED` | BED400 | 4448 | `f9ffa0abff120b243b99b3d6006697becb10735fc24f0fb798beab2a467ece7a` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::ATC_J_J.BED` | BED400 | 3616 | `b70ab3abd0d5491ba55fc2cb9ddfc918225ba0090e58e3aed9d396e628ce6dd3` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 6560 | `fa971120bb56cc962a0792d7af2a572779c23fa18769b814c5a531dc7e09f97f` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D_J.BED` | BED400 | 4448 | `f9ffa0abff120b243b99b3d6006697becb10735fc24f0fb798beab2a467ece7a` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J_J.BED` | BED400 | 3616 | `b70ab3abd0d5491ba55fc2cb9ddfc918225ba0090e58e3aed9d396e628ce6dd3` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 6560 | `fa971120bb56cc962a0792d7af2a572779c23fa18769b814c5a531dc7e09f97f` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_es.cpk::battle/boss/e200.bin::BED/e200_ENCOUNT.BED` | BED400 | 14288 | `4294f933ecf65bfba4e1b444999c903c3c47a29462602e79b53ed7a2e7ceb452` |
| `data_es.cpk::battle/boss/e200.bin::BED/e200_END.BED` | BED400 | 33088 | `931f2c31d66a9853504d6ad85b72593d9e62da352f97719a930c8bed0e3ab733` |
| `data_es.cpk::battle/boss/e200.bin::BED/e200_H160.BED` | BED400 | 9584 | `1a0d373c01cb5744cd98f6268da56e265539fe9b10a18a20d880348c77bc727a` |
| `data_es.cpk::battle/boss/e200.bin::BED/e200_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_es.cpk::battle/boss/e200.bin::BED/e200_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e200.bin::BED/e200_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e200.bin::BED/e200_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e200.bin::BED/e200_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e201.bin::BED/e201_ENCOUNT.BED` | BED400 | 10080 | `cf6ee31d878f652f19a197d9b18502f5e32e4c765131066088aa7a008feda7bb` |
| `data_es.cpk::battle/boss/e201.bin::BED/e201_END.BED` | BED400 | 30192 | `b31d78e5de0092924773b1d1b2e55a1955a77c1e2175c64ea794dba20d1c32fd` |
| `data_es.cpk::battle/boss/e201.bin::BED/e201_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_es.cpk::battle/boss/e201.bin::BED/e201_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e201.bin::BED/e201_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e201.bin::BED/e201_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e201.bin::BED/e201_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e202.bin::BED/e202_ENCOUNT.BED` | BED400 | 7344 | `5802ab6fc50884bbc4e1db4f97b92b60079582f1126c2ad60cc8e44567c4fc8c` |
| `data_es.cpk::battle/boss/e202.bin::BED/e202_END.BED` | BED400 | 30992 | `28078663ea649edae5bcd52b50f6e614bfb54f41f722d5d18654ea082fe50bc6` |
| `data_es.cpk::battle/boss/e202.bin::BED/e202_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_es.cpk::battle/boss/e202.bin::BED/e202_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e202.bin::BED/e202_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e202.bin::BED/e202_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e202.bin::BED/e202_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e203.bin::BED/e203_ENCOUNT.BED` | BED400 | 8960 | `97081976c63beb477742d9e10a2a83bf02c08fbac7c5e5a0c727b332304cf803` |
| `data_es.cpk::battle/boss/e203.bin::BED/e203_END.BED` | BED400 | 35536 | `6221812dc3626928a9ab734835fce955f56d3164995195624b7d20907e249863` |
| `data_es.cpk::battle/boss/e203.bin::BED/e203_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_es.cpk::battle/boss/e203.bin::BED/e203_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e203.bin::BED/e203_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e203.bin::BED/e203_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e203.bin::BED/e203_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_es.cpk::battle/boss/e203.bin::BED/e203_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e204.bin::BED/e204_ENCOUNT.BED` | BED400 | 9152 | `2b707e32159c57f61dda2a51f9a4d1bb13dbaff059eb0b294294771e29b2c155` |
| `data_es.cpk::battle/boss/e204.bin::BED/e204_END.BED` | BED400 | 7040 | `ff3be583ba835a33b94a87a47080141ccda1aaa9cbe5946ce80a861386da0534` |
| `data_es.cpk::battle/boss/e204.bin::BED/e204_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_es.cpk::battle/boss/e204.bin::BED/e204_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e204.bin::BED/e204_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e204.bin::BED/e204_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e204.bin::BED/e204_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_es.cpk::battle/boss/e204.bin::BED/e204_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e205.bin::BED/e205_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e205.bin::BED/e205_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e205.bin::BED/e205_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e205.bin::BED/e205_ENCOUNT.BED` | BED400 | 10336 | `2f5fad7c4101340a9f82380c62dc0341cf2218734ebcfb157e4f68be0eca017b` |
| `data_es.cpk::battle/boss/e205.bin::BED/e205_END.BED` | BED400 | 32320 | `151da6e1d4d39174971a53d45f228d2ef3e60e9513815541c9b7dc0e03187fe1` |
| `data_es.cpk::battle/boss/e205.bin::BED/e205_HATUDO.BED` | BED400 | 3696 | `18feb0b9a37f5d925e6a17a49030adb5b2280a2cb28fb6261a6dd1460507cdb0` |
| `data_es.cpk::battle/boss/e205.bin::BED/e205_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e205.bin::BED/e205_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e205.bin::BED/e205_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e205.bin::BED/e205_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_es.cpk::battle/boss/e205.bin::BED/e205_KAIJYO.BED` | BED400 | 5152 | `99bfb77d8d42d84c48c89e01526da14dd68075d99521a820c0675e0450e1d373` |
| `data_es.cpk::battle/boss/e205.bin::BED/e205_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e206.bin::BED/e206_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e206.bin::BED/e206_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e206.bin::BED/e206_ENCOUNT.BED` | BED400 | 6400 | `5b9ca9679564c387f222fcd3aefbba7ca6b0719893a33f2f14f35e1d09b064ce` |
| `data_es.cpk::battle/boss/e206.bin::BED/e206_END.BED` | BED400 | 31504 | `ee15eb497f11a707b85ad4c857527ba2732eaf4f9936fce305bbb4056de1d85d` |
| `data_es.cpk::battle/boss/e206.bin::BED/e206_H172.BED` | BED400 | 7552 | `74a7944243d6c20da666245d7ba3d0e76b957cb06136a5de79bbf03cd9c5f1bb` |
| `data_es.cpk::battle/boss/e206.bin::BED/e206_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_es.cpk::battle/boss/e206.bin::BED/e206_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e206.bin::BED/e206_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e206.bin::BED/e206_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e206.bin::BED/e206_H_SP.BED` | BED400 | 4064 | `7007fbcdd175abbf881d3ae61fe12bed93872c08931f3e6b82aed5bfc83a2dc7` |
| `data_es.cpk::battle/boss/e206.bin::BED/e206_OFF.BED` | BED400 | 8208 | `3a6e572de45506664a54fa77e2f355094be47cc77a6b603306bad35f8b940175` |
| `data_es.cpk::battle/boss/e206.bin::BED/e206_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e207.bin::BED/e207_ENCOUNT.BED` | BED400 | 6944 | `e5db68919f29386aa0f2142e7da82e62bea35af0985694fb2a2ded112255088f` |
| `data_es.cpk::battle/boss/e207.bin::BED/e207_END.BED` | BED400 | 32096 | `cc1f9d2b5dec6f8e5b3f3fa82b5710399537f04c31bb09896f4cee7a11e8d383` |
| `data_es.cpk::battle/boss/e207.bin::BED/e207_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_es.cpk::battle/boss/e207.bin::BED/e207_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e207.bin::BED/e207_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e207.bin::BED/e207_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e207.bin::BED/e207_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e208.bin::BED/e208_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e208.bin::BED/e208_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e208.bin::BED/e208_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e208.bin::BED/e208_ENCOUNT.BED` | BED400 | 14032 | `2275fd23ee2bea212640b51f317d363e17a98e93db834cba7a104090bb652b60` |
| `data_es.cpk::battle/boss/e208.bin::BED/e208_END.BED` | BED400 | 32336 | `b4a227dc5312f1dc2e4df9c6c0e5c11216ee2c24a029b1b7a209b50757dd2bde` |
| `data_es.cpk::battle/boss/e208.bin::BED/e208_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_es.cpk::battle/boss/e208.bin::BED/e208_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e208.bin::BED/e208_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e208.bin::BED/e208_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e208.bin::BED/e208_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_es.cpk::battle/boss/e208.bin::BED/e208_SK17B.BED` | BED400 | 13328 | `0d0a8231a695a1e9497d98246dfc0c544a03b95f827f3a45adb2fd3ffeb7a704` |
| `data_es.cpk::battle/boss/e208.bin::BED/e208_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e209.bin::BED/e209_ENCOUNT.BED` | BED400 | 9392 | `80719c0c13a8fde7ff0e545957665da6cffd72631c30e8525e0962927f919266` |
| `data_es.cpk::battle/boss/e209.bin::BED/e209_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data_es.cpk::battle/boss/e209.bin::BED/e209_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_es.cpk::battle/boss/e209.bin::BED/e209_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_es.cpk::battle/boss/e209.bin::BED/e209_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_es.cpk::battle/boss/e209.bin::BED/e209_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_es.cpk::battle/boss/e209.bin::BED/e209_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20A.bin::BED/e20A_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20A.bin::BED/e20A_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20A.bin::BED/e20A_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20A.bin::BED/e20A_ENCOUNT.BED` | BED400 | 7248 | `7ce81d540666b51712f628a3ace38d282202ae8035cd6c497bcba877582a0d26` |
| `data_es.cpk::battle/boss/e20A.bin::BED/e20A_END.BED` | BED400 | 30736 | `0a7fd4451e4224bbd65059a54872ed290c3a637eff66b564a19a7cafb69e82ec` |
| `data_es.cpk::battle/boss/e20A.bin::BED/e20A_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_es.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20A.bin::BED/e20A_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_es.cpk::battle/boss/e20A.bin::BED/e20A_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20B.bin::BED/e20B_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20B.bin::BED/e20B_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20B.bin::BED/e20B_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT.BED` | BED400 | 7408 | `bfbcd65ffdff97966112ba29739ca11224d5130bd9e24793232706a5d78b80c3` |
| `data_es.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT2.BED` | BED400 | 6976 | `f14a2a742989b86d727615b805d73d35b92cfa2e98e4f1f2715b33c7850086aa` |
| `data_es.cpk::battle/boss/e20B.bin::BED/e20B_END.BED` | BED400 | 20400 | `fd46f5973ff92fc5d781a282fb094494df3e8a55809730bd91d623b8b55180d8` |
| `data_es.cpk::battle/boss/e20B.bin::BED/e20B_END1.BED` | BED400 | 52608 | `8069c2bcc904b6fa9ed727768508d39a56ecc95f3ce509fbb4d92d5c9c1073fb` |
| `data_es.cpk::battle/boss/e20B.bin::BED/e20B_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_es.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20B.bin::BED/e20B_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_es.cpk::battle/boss/e20B.bin::BED/e20B_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20C.bin::BED/e20C_ENCOUNT.BED` | BED400 | 8016 | `0d7531f757e8a743070f66980adba7097a34223c7ceab5b7e15e1006db2b0f76` |
| `data_es.cpk::battle/boss/e20C.bin::BED/e20C_END.BED` | BED400 | 23760 | `98e314cd795aabd6134413891b58afcd5b06a5fa4c2668ab5014bb7c9e19ec9f` |
| `data_es.cpk::battle/boss/e20C.bin::BED/e20C_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_es.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20C.bin::BED/e20C_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e20D.bin::BED/e20D_ENCOUNT.BED` | BED400 | 6592 | `d23955927d2d12aa710e56458ce8d64c9b172cedcea6c36cd99c5696fb1d049f` |
| `data_es.cpk::battle/boss/e20D.bin::BED/e20D_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data_es.cpk::battle/boss/e20D.bin::BED/e20D_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_es.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_es.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_es.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_es.cpk::battle/boss/e20D.bin::BED/e20D_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e214.bin::BED/e214_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e214.bin::BED/e214_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e214.bin::BED/e214_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e214.bin::BED/e214_ENCOUNT.BED` | BED400 | 10176 | `b56f07d78e20e9c4dbd1adc3f9c582035dd213c4a751d4a6df5889ddd7492a3d` |
| `data_es.cpk::battle/boss/e214.bin::BED/e214_END.BED` | BED400 | 5696 | `d109a2a14e0ee8b4fd4764e6c08aa8b78cb68e76651a42ac48c3638a0361576a` |
| `data_es.cpk::battle/boss/e214.bin::BED/e214_HATUDO.BED` | BED400 | 3504 | `b1792d9bd7867be710d47f27d81220d79495ec12d027191ddcd917f963e0ead6` |
| `data_es.cpk::battle/boss/e214.bin::BED/e214_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e214.bin::BED/e214_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e214.bin::BED/e214_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e214.bin::BED/e214_H_SP.BED` | BED400 | 3696 | `122de1e6d1504bf72f5f0fb267dc05aa3e80483c32cf8db7e8330feb9cfcce82` |
| `data_es.cpk::battle/boss/e214.bin::BED/e214_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e215.bin::BED/e215_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e215.bin::BED/e215_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e215.bin::BED/e215_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e215.bin::BED/e215_ENCOUNT.BED` | BED400 | 11296 | `e99453fa2913f5bbcad4d7c7b3ac29df97d5d35f5c84188af83b20bc5f4b35cf` |
| `data_es.cpk::battle/boss/e215.bin::BED/e215_END.BED` | BED400 | 1230864 | `c30afdd4e4cf82c8eebe4aa26faf5893a3427e5bd1595dd635f9aa63e89c9fc5` |
| `data_es.cpk::battle/boss/e215.bin::BED/e215_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_es.cpk::battle/boss/e215.bin::BED/e215_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e215.bin::BED/e215_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e215.bin::BED/e215_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e215.bin::BED/e215_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_es.cpk::battle/boss/e215.bin::BED/e215_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e216.bin::BED/e216_ENCOUNT.BED` | BED400 | 11200 | `1dcc41a7929352eb50ba07e44f68d24ed1c0a12e8cbe954e7cb75f735f1da34e` |
| `data_es.cpk::battle/boss/e216.bin::BED/e216_END.BED` | BED400 | 10576 | `49b1c46532ed9d3cc5a87e47736efd0ea4ec0584910f0d09d6b57ad1153d3292` |
| `data_es.cpk::battle/boss/e216.bin::BED/e216_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_es.cpk::battle/boss/e216.bin::BED/e216_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e216.bin::BED/e216_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e216.bin::BED/e216_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e216.bin::BED/e216_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_es.cpk::battle/boss/e216.bin::BED/e216_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/boss/e217.bin::BED/e217_ENCOUNT.BED` | BED400 | 6304 | `959dca87cf9853542ee07ada31ffde9a842967b3194b9a38fec7e0e912fac55c` |
| `data_es.cpk::battle/boss/e217.bin::BED/e217_END.BED` | BED400 | 60592 | `ea818bdc095b1797f4927266ae081698520a0b3e4b3f4c4d33693bfd57d28f82` |
| `data_es.cpk::battle/boss/e217.bin::BED/e217_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_es.cpk::battle/boss/e217.bin::BED/e217_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_es.cpk::battle/boss/e217.bin::BED/e217_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_es.cpk::battle/boss/e217.bin::BED/e217_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_es.cpk::battle/boss/e217.bin::BED/e217_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_es.cpk::battle/result/shuffle.arc::SHUFFLE01.EPL` | EPL | 6368 | `bb7e2d674c4b18778f6f7b02304159ee7a192b90f38406353bd32754207244b5` |
| `data_es.cpk::battle/result/shuffle.arc::SHUFF_01.EPL` | EPL | 8864 | `59627e77df24e98b1aec5cf22a9c47495e145505c6fea06b5b0429b3427a7fad` |
| `data_es.cpk::battle/result/shuffle.arc::SHUFF_02.EPL` | EPL | 9760 | `58c0739b468638264c6d3534bd2b2c6f5af07a3e6d878fc4104351a4f01b9dad` |
| `data_es.cpk::battle/result/shuffle.arc::SHUFF_03.EPL` | EPL | 14688 | `c50a3a6c14cb67013b24aa7f12833c401ef5ef7f5bb823ec4a45916b1d996782` |
| `data_es.cpk::battle/result/shuffle.arc::SHUFF_04.EPL` | EPL | 9568 | `2ac489ffa035960b485696279cdcdc702e512321dfec3c5de3e0533354ac27ef` |
| `data_es.cpk::battle/result/shuffle.arc::SHUFF_05.EPL` | EPL | 10144 | `1c02f0750e4a6e99932d04af21a9752ae50b80597201fbc7ce944bdde2b45432` |
| `data_es.cpk::battle/result/shuffle.arc::SHUFF_06.EPL` | EPL | 15104 | `a7756c94b0471c8bedc1447c499df3233be21f8992655205858fb1e97e7d7c96` |
| `data_es.cpk::battle/result/shuffle.arc::SHUFF_09.EPL` | EPL | 1600 | `7db6177b81d30997b063517caf3b0a1a1781a7363f3219940aaa1d3deef01bb4` |
| `data_es.cpk::event/fishing.bin::FLOAT_01.EPL` | EPL | 7232 | `3f7721ea180386dc79379572ea567e7521b2a470d9f76167aa71bbdebdb3f5d0` |
| `data_es.cpk::event/fishing.bin::FLOAT_WV.EPL` | EPL | 7264 | `db0358e93caaab0ccfe666c308ece5ff9acc4522ae95044fbc00be249dadb346` |
| `data_es.cpk::event/fishing.bin::SPRAY_01.EPL` | EPL | 11424 | `6229b4e3eae59c7f62ee785737578b3aba127205f979dd528d31222cb60d0be3` |
| `data_es.cpk::event/fishing.bin::SPRAY_02.EPL` | EPL | 4288 | `54cabd45d19febb6d5decaeb2ee9c081f8c34a1e3273c2db20449cc0fb637aa1` |
| `data_es.cpk::event/fishing.bin::SPRAY_03.EPL` | EPL | 23296 | `34836ad2132b20984fab062cf62efbae171cd5438d7d64d11d3dbe06661d15f3` |
| `data_es.cpk::event/fishing.bin::SPRAY_04.EPL` | EPL | 22880 | `9d383ff669c401e9ff458d0904761040d95cfbd90b64b6aef2bf04dded321238` |
| `data_es.cpk::event/fishing.bin::WAVE_01.EPL` | EPL | 512 | `e73f641168a4d70f2cfcb10cf9d4bbb71fcf1466208de78c682d22e8ba876bb3` |
| `data_es.cpk::event/fishing.bin::WAVE_02.EPL` | EPL | 512 | `e623b56cce128bbc917f276a6238991b9417971c74de892194acc4e17610a982` |
| `data_es.cpk::event/fishing.bin::WAVE_03.EPL` | EPL | 864 | `7431335642b1c77dc423964534743a10b1c9193279f2e36078cba7315e7a8fd4` |
| `data_es.cpk::event/fishing.bin::WAVE_04.EPL` | EPL | 82368 | `5975b6978e545c03916310565458748458e384874ecd227887f43dc0eb331c5f` |
| `data_es.cpk::event/fishing.bin::WAVE_05.EPL` | EPL | 14752 | `362d3e3b9d86e96cc408b2ac392838551848ecb27d829fe2996d1581b7132cfc` |
| `data_es.cpk::event/fishing.bin::WAVE_06.EPL` | EPL | 15232 | `7f2fc8f2c4b6baf6b48cecabdfb1ab1c09b3971cab53ea9933f4df36846d71f9` |
| `data_es.cpk::event_data/community.bin::CARD_30.EPL` | EPL | 267120 | `51a878d3a3d380b26abeab633ad5e075894c519e3fa3436cce36af0d81cc6c91` |
| `data_es.cpk::event_data/community.bin::CARD_EF04.EPL` | EPL | 153600 | `feb474459c1fb4b4d3b11ea541d82ea2673a32a14118275c43e0430a21ec9311` |
| `data_es.cpk::event_data/community.bin::CARD_EF06.EPL` | EPL | 134112 | `cb1ed7c4957fdbd26b0081735cb9b4c0c55587eeb604c5d868c972e738011621` |
| `data_es.cpk::event_data/community.bin::CARD_EF07.EPL` | EPL | 19440 | `2b55441af548b5b3c90bc1bad8253942d9f68fe7cfa2cdf75779d5f0dcbc86f4` |
| `data_es.cpk::event_data/community.bin::CMY01_02.EPL` | EPL | 7184 | `4d4eee955d10ab94f0672798d119f60466cb4437c3027c45ad8d13c1cd826b73` |
| `data_es.cpk::event_data/community.bin::CMY02_01.EPL` | EPL | 7232 | `b5561bee072cbe2532eb2829bf5c39fc91927f53fdf184e65a67b8d01afc5dfe` |
| `data_es.cpk::event_data/community.bin::CMY04_06.EPL` | EPL | 2816 | `1d99560e14f4d5304bad68a200e428f909d7b806466b1e504b7c8cb1a5b79800` |
| `data_es.cpk::event_data/community.bin::CMY05_05.EPL` | EPL | 13440 | `4252a6171efe301668d8d50dc48ad1d5157e8fb7ea0e10e1322c31690c4747e1` |
| `data_es.cpk::event_data/community.bin::E_KIRA.EPL` | EPL | 12560 | `9b83128cbed2ba53cb6c6bea178e6a26ba031a32db7e39f0863e09adac8b5e9c` |
| `data_es.cpk::event_data/community.bin::E_KIRA2.EPL` | EPL | 12560 | `cbc82cec6fd117a07c16d78d18265390c36c0ec058022e0f36fba5e944fabf85` |
| `data_es.cpk::event_data/community.bin::P_GET3.EPL` | EPL | 2128 | `928fd9aaecd4c39732a2c2492670dac71de00a6dda594d74c2316286a5127c9d` |
| `data_es.cpk::facility/cmbroot.arc::E_PER.EPL` | EPL | 1120 | `07b9bac782b287964bc963b576ddc16a1724ce44010e3bf9a86f6972a757a1a9` |
| `data_es.cpk::facility/cmbroot.arc::GAT07_01.EPL` | EPL | 19840 | `367b85f1f9785cb0a1aace1573b309b9a9eecf9df2d174eef2d78cdc332c8d16` |
| `data_es.cpk::facility/cmbroot.arc::PER3.EPL` | EPL | 640 | `a8420c4ae86d038a3f45da5fed5afb3bcc040783e1aa5372c1957916fb5bf9b8` |
| `data_es.cpk::field/pack/fd031_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_es.cpk::field/pack/fd031_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_es.cpk::field/pack/fd031_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_es.cpk::field/pack/fd031_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_es.cpk::field/pack/fd069_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_es.cpk::field/pack/fd069_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_es.cpk::field/pack/fd069_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_es.cpk::field/pack/fd069_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_es.cpk::field/pack/fd069_003.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_es.cpk::field/pack/fd069_003.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_es.cpk::field/pack/fd069_004.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_es.cpk::field/pack/fd069_004.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON1.EPL` | EPL | 35360 | `419957b5504a91136e2a6da0a9191adbb0d0fdcabec7745d7c6a0245337e1a0c` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON10.EPL` | EPL | 640 | `a459f3d6a49e217485c18351e1be0074790389f619fa11a21c41d10f817a629e` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON11.EPL` | EPL | 35360 | `a2b85ec8fd28435027137c00eb3db2613488890e426eb15950693e450ae3666e` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON12.EPL` | EPL | 13920 | `7c600dbf814ca52978843c10998ac5877290426bce5fc15b11d188ced6dc7655` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON13.EPL` | EPL | 2816 | `4c470d96fa640beb21f4b5c5f69da7278816df340150539b19a6d282b58d3745` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON14.EPL` | EPL | 3200 | `44511407c6c60dcb3fdffc2ea0b295f3e367373ad4e47fafb8f68ffbfa523742` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON15.EPL` | EPL | 3776 | `f376c7bd0d6fd7aa6614093b86b47465f5ce418e4456293015687a845851c6c0` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON16.EPL` | EPL | 8960 | `749535ddb03ac80e9c173a4e2c4e011bb445ef4c3afea446d9327356e03e53e8` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON17.EPL` | EPL | 5408 | `682ce6151e173a4e5d837be7324c60c7e5475112682d4f7734f32c2200316fa1` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON18.EPL` | EPL | 2816 | `ac6cae65586ea536872e38741e8d1b4f710faaa61209c23a54c2daf629f38528` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON19.EPL` | EPL | 10784 | `1712a4d006ad15b02ac959902c2ea753f852832ec6d94c459ca0dd92b2f31ed1` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON2.EPL` | EPL | 70560 | `d1f15c72717d7ae5b2b5b495c209f9ce6928d98bdd7a0a9e0d45e6c95ba8e12b` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON20.EPL` | EPL | 11456 | `3c36b0b747116189a73e67773c05f474611512d39db9f7a81c47ce562f40632e` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON21.EPL` | EPL | 16768 | `ca32d5e7033db22860116d6accd7983ace3ee5c840f8ef936f22566b987bca92` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON22.EPL` | EPL | 4448 | `d0d9ad27c09e15df5caccb17467b71e5b15ba39d20f2c325d4d445caa7aac979` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON23.EPL` | EPL | 7328 | `83a7d4ae49cc4d9237798ed7eb905ef31e237550537bd89f67cf5744fbf8d102` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON24.EPL` | EPL | 13440 | `91538d6b1c7770ad8a4b4a2c34556939c9c1e4f1fc1beede20f8ba87ac5d5e7a` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON25.EPL` | EPL | 52960 | `5d8963dec880d512cc7453b46e2c0187e4113a21f82a431656472a48467951e5` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON26.EPL` | EPL | 2816 | `37621fb4cae82626436d81fabdc9b4177905063ac5264703e7de25f58e06bad3` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON3.EPL` | EPL | 12928 | `c8174ba08c05c1d7c62cb57138c3487e376dcf16713d3341a6018fc413e46688` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON4.EPL` | EPL | 32928 | `2f173d83c1e9a766c6bd5f21b90812ba376f44e2c7fd4b314b3e8c3004087b64` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON5.EPL` | EPL | 10464 | `72c96b08b6fdf5fcc5aad1a3dc4961b30e063647d1c8bac2a51ec464789c7bdd` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON6.EPL` | EPL | 24064 | `9115e4344e6efa200a9b2bad425236d6c514a6f4109e1154b080140c0889cd1b` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON7.EPL` | EPL | 2816 | `dedee54a4f210c6a3e993d17ed78bbc896a6c1e97acafc1e50e21b7ceb78f149` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON8.EPL` | EPL | 5472 | `15038b74e6204b01c8f1fee6595dba30b926e4a37e052bc8de8f53b10fb77dec` |
| `data_es.cpk::init.bin::event/icon.bin::_ICON9.EPL` | EPL | 34688 | `f4970730444758d9b1e2c20933ecdba4d9c69a92422d4722eb33caf384c0491b` |
| `data_es.cpk::init.bin::field/F_BLUR.EPL` | EPL | 640 | `f7c0c9b76c0cc4cb11dbe0816d1d3d83ef72ccd00f33a9480acf0ecd55fcfb6c` |
| `data_es.cpk::init.bin::field/F_ICON01.EPL` | EPL | 11856 | `bb0d1adabff9e8808efb526a1d5ed9a438e00209958956d7fd8b83d811836882` |
| `data_es.cpk::init.bin::field/F_ICON02.EPL` | EPL | 11312 | `37486846f11641eae744873620fd47689a2b343adebd53a567287c1918007555` |
| `data_es.cpk::init.bin::field/F_ICON03.EPL` | EPL | 11952 | `9f58f431f3147529befe48618a378f464538847caf9d1ac7ec6cf0fb2a1c21c7` |
| `data_es.cpk::init.bin::field/F_ICON04.EPL` | EPL | 11312 | `a2b0a94439e8716ecece6426d24874563ad7d39294abd923b21c226ca890eeb7` |
| `data_es.cpk::init.bin::field/F_ICON05.EPL` | EPL | 11312 | `24aa952f33209549dd9a2c0ed01f2bf1f9e1f3d1883fcd1bdcdca959d4c827ca` |
| `data_es.cpk::init.bin::field/RAIN_01.EPL` | EPL | 9440 | `af7e203dbc36d37d53577a673f20b9a6b71fa5bd1b52189326d4ea476e560c5e` |
| `data_es.cpk::init.bin::field/SNOW_01.EPL` | EPL | 640 | `4c614a3d103ad5745d5610865f9e2cdac2fc4b90c04b343d1f3ff61401c0bba1` |
| `data_es.cpk::init.bin::field/SNOW_02.EPL` | EPL | 640 | `3f025061842c24fa748cdd09adabd7de493c95eae332e544557efd0602798454` |
| `data_es.cpk::init.bin::field/effect/BREATH_01.EPL` | EPL | 640 | `9e5da14b05e6803b490c3f6e71ceac80965aaba7184b3ce317229ecf53617630` |
| `data_es.cpk::init.bin::field/effect/FOOT_L.EPL` | EPL | 5728 | `e4169139debccf8f28e88e86e2b72a49e10ecfee15a61eb745fd1a1bf1f18eee` |
| `data_es.cpk::init.bin::field/effect/FOOT_R.EPL` | EPL | 5728 | `83828867762a5b9ed87f11eb45f91284974350052e85d8993a737fc0229e12a2` |
| `data_es.cpk::init.bin::insect/insect.bin::INSECT_01.EPL` | EPL | 9696 | `74be2671465a4e5e760c6697cd27f5e96dce151f48b488fc40262c75cda40ea7` |
| `data_es.cpk::init_free.bin::battle/effect/SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::ATC_D_J.BED` | BED400 | 4448 | `f9ffa0abff120b243b99b3d6006697becb10735fc24f0fb798beab2a467ece7a` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::ATC_J_J.BED` | BED400 | 3616 | `b70ab3abd0d5491ba55fc2cb9ddfc918225ba0090e58e3aed9d396e628ce6dd3` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 6560 | `fa971120bb56cc962a0792d7af2a572779c23fa18769b814c5a531dc7e09f97f` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D_J.BED` | BED400 | 4448 | `f9ffa0abff120b243b99b3d6006697becb10735fc24f0fb798beab2a467ece7a` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J_J.BED` | BED400 | 3616 | `b70ab3abd0d5491ba55fc2cb9ddfc918225ba0090e58e3aed9d396e628ce6dd3` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 6560 | `fa971120bb56cc962a0792d7af2a572779c23fa18769b814c5a531dc7e09f97f` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_fr.cpk::battle/boss/e200.bin::BED/e200_ENCOUNT.BED` | BED400 | 14288 | `4294f933ecf65bfba4e1b444999c903c3c47a29462602e79b53ed7a2e7ceb452` |
| `data_fr.cpk::battle/boss/e200.bin::BED/e200_END.BED` | BED400 | 33088 | `931f2c31d66a9853504d6ad85b72593d9e62da352f97719a930c8bed0e3ab733` |
| `data_fr.cpk::battle/boss/e200.bin::BED/e200_H160.BED` | BED400 | 9584 | `1a0d373c01cb5744cd98f6268da56e265539fe9b10a18a20d880348c77bc727a` |
| `data_fr.cpk::battle/boss/e200.bin::BED/e200_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_fr.cpk::battle/boss/e200.bin::BED/e200_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e200.bin::BED/e200_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e200.bin::BED/e200_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e200.bin::BED/e200_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e201.bin::BED/e201_ENCOUNT.BED` | BED400 | 10080 | `cf6ee31d878f652f19a197d9b18502f5e32e4c765131066088aa7a008feda7bb` |
| `data_fr.cpk::battle/boss/e201.bin::BED/e201_END.BED` | BED400 | 30192 | `b31d78e5de0092924773b1d1b2e55a1955a77c1e2175c64ea794dba20d1c32fd` |
| `data_fr.cpk::battle/boss/e201.bin::BED/e201_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_fr.cpk::battle/boss/e201.bin::BED/e201_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e201.bin::BED/e201_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e201.bin::BED/e201_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e201.bin::BED/e201_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e202.bin::BED/e202_ENCOUNT.BED` | BED400 | 7344 | `5802ab6fc50884bbc4e1db4f97b92b60079582f1126c2ad60cc8e44567c4fc8c` |
| `data_fr.cpk::battle/boss/e202.bin::BED/e202_END.BED` | BED400 | 30992 | `28078663ea649edae5bcd52b50f6e614bfb54f41f722d5d18654ea082fe50bc6` |
| `data_fr.cpk::battle/boss/e202.bin::BED/e202_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_fr.cpk::battle/boss/e202.bin::BED/e202_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e202.bin::BED/e202_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e202.bin::BED/e202_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e202.bin::BED/e202_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e203.bin::BED/e203_ENCOUNT.BED` | BED400 | 8960 | `97081976c63beb477742d9e10a2a83bf02c08fbac7c5e5a0c727b332304cf803` |
| `data_fr.cpk::battle/boss/e203.bin::BED/e203_END.BED` | BED400 | 35536 | `6221812dc3626928a9ab734835fce955f56d3164995195624b7d20907e249863` |
| `data_fr.cpk::battle/boss/e203.bin::BED/e203_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_fr.cpk::battle/boss/e203.bin::BED/e203_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e203.bin::BED/e203_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e203.bin::BED/e203_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e203.bin::BED/e203_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_fr.cpk::battle/boss/e203.bin::BED/e203_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e204.bin::BED/e204_ENCOUNT.BED` | BED400 | 9152 | `2b707e32159c57f61dda2a51f9a4d1bb13dbaff059eb0b294294771e29b2c155` |
| `data_fr.cpk::battle/boss/e204.bin::BED/e204_END.BED` | BED400 | 7040 | `ff3be583ba835a33b94a87a47080141ccda1aaa9cbe5946ce80a861386da0534` |
| `data_fr.cpk::battle/boss/e204.bin::BED/e204_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_fr.cpk::battle/boss/e204.bin::BED/e204_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e204.bin::BED/e204_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e204.bin::BED/e204_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e204.bin::BED/e204_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_fr.cpk::battle/boss/e204.bin::BED/e204_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e205.bin::BED/e205_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e205.bin::BED/e205_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e205.bin::BED/e205_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e205.bin::BED/e205_ENCOUNT.BED` | BED400 | 10336 | `2f5fad7c4101340a9f82380c62dc0341cf2218734ebcfb157e4f68be0eca017b` |
| `data_fr.cpk::battle/boss/e205.bin::BED/e205_END.BED` | BED400 | 32320 | `151da6e1d4d39174971a53d45f228d2ef3e60e9513815541c9b7dc0e03187fe1` |
| `data_fr.cpk::battle/boss/e205.bin::BED/e205_HATUDO.BED` | BED400 | 3696 | `18feb0b9a37f5d925e6a17a49030adb5b2280a2cb28fb6261a6dd1460507cdb0` |
| `data_fr.cpk::battle/boss/e205.bin::BED/e205_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e205.bin::BED/e205_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e205.bin::BED/e205_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e205.bin::BED/e205_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_fr.cpk::battle/boss/e205.bin::BED/e205_KAIJYO.BED` | BED400 | 5152 | `99bfb77d8d42d84c48c89e01526da14dd68075d99521a820c0675e0450e1d373` |
| `data_fr.cpk::battle/boss/e205.bin::BED/e205_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e206.bin::BED/e206_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e206.bin::BED/e206_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e206.bin::BED/e206_ENCOUNT.BED` | BED400 | 6400 | `5b9ca9679564c387f222fcd3aefbba7ca6b0719893a33f2f14f35e1d09b064ce` |
| `data_fr.cpk::battle/boss/e206.bin::BED/e206_END.BED` | BED400 | 31504 | `ee15eb497f11a707b85ad4c857527ba2732eaf4f9936fce305bbb4056de1d85d` |
| `data_fr.cpk::battle/boss/e206.bin::BED/e206_H172.BED` | BED400 | 7552 | `74a7944243d6c20da666245d7ba3d0e76b957cb06136a5de79bbf03cd9c5f1bb` |
| `data_fr.cpk::battle/boss/e206.bin::BED/e206_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_fr.cpk::battle/boss/e206.bin::BED/e206_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e206.bin::BED/e206_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e206.bin::BED/e206_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e206.bin::BED/e206_H_SP.BED` | BED400 | 4064 | `7007fbcdd175abbf881d3ae61fe12bed93872c08931f3e6b82aed5bfc83a2dc7` |
| `data_fr.cpk::battle/boss/e206.bin::BED/e206_OFF.BED` | BED400 | 8208 | `3a6e572de45506664a54fa77e2f355094be47cc77a6b603306bad35f8b940175` |
| `data_fr.cpk::battle/boss/e206.bin::BED/e206_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e207.bin::BED/e207_ENCOUNT.BED` | BED400 | 6944 | `e5db68919f29386aa0f2142e7da82e62bea35af0985694fb2a2ded112255088f` |
| `data_fr.cpk::battle/boss/e207.bin::BED/e207_END.BED` | BED400 | 32096 | `cc1f9d2b5dec6f8e5b3f3fa82b5710399537f04c31bb09896f4cee7a11e8d383` |
| `data_fr.cpk::battle/boss/e207.bin::BED/e207_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_fr.cpk::battle/boss/e207.bin::BED/e207_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e207.bin::BED/e207_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e207.bin::BED/e207_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e207.bin::BED/e207_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e208.bin::BED/e208_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e208.bin::BED/e208_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e208.bin::BED/e208_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e208.bin::BED/e208_ENCOUNT.BED` | BED400 | 14032 | `2275fd23ee2bea212640b51f317d363e17a98e93db834cba7a104090bb652b60` |
| `data_fr.cpk::battle/boss/e208.bin::BED/e208_END.BED` | BED400 | 32336 | `b4a227dc5312f1dc2e4df9c6c0e5c11216ee2c24a029b1b7a209b50757dd2bde` |
| `data_fr.cpk::battle/boss/e208.bin::BED/e208_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_fr.cpk::battle/boss/e208.bin::BED/e208_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e208.bin::BED/e208_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e208.bin::BED/e208_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e208.bin::BED/e208_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_fr.cpk::battle/boss/e208.bin::BED/e208_SK17B.BED` | BED400 | 13328 | `0d0a8231a695a1e9497d98246dfc0c544a03b95f827f3a45adb2fd3ffeb7a704` |
| `data_fr.cpk::battle/boss/e208.bin::BED/e208_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e209.bin::BED/e209_ENCOUNT.BED` | BED400 | 9392 | `80719c0c13a8fde7ff0e545957665da6cffd72631c30e8525e0962927f919266` |
| `data_fr.cpk::battle/boss/e209.bin::BED/e209_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data_fr.cpk::battle/boss/e209.bin::BED/e209_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_fr.cpk::battle/boss/e209.bin::BED/e209_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_fr.cpk::battle/boss/e209.bin::BED/e209_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_fr.cpk::battle/boss/e209.bin::BED/e209_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_fr.cpk::battle/boss/e209.bin::BED/e209_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20A.bin::BED/e20A_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20A.bin::BED/e20A_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20A.bin::BED/e20A_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20A.bin::BED/e20A_ENCOUNT.BED` | BED400 | 7248 | `7ce81d540666b51712f628a3ace38d282202ae8035cd6c497bcba877582a0d26` |
| `data_fr.cpk::battle/boss/e20A.bin::BED/e20A_END.BED` | BED400 | 30736 | `0a7fd4451e4224bbd65059a54872ed290c3a637eff66b564a19a7cafb69e82ec` |
| `data_fr.cpk::battle/boss/e20A.bin::BED/e20A_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_fr.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20A.bin::BED/e20A_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_fr.cpk::battle/boss/e20A.bin::BED/e20A_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20B.bin::BED/e20B_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20B.bin::BED/e20B_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20B.bin::BED/e20B_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT.BED` | BED400 | 7408 | `bfbcd65ffdff97966112ba29739ca11224d5130bd9e24793232706a5d78b80c3` |
| `data_fr.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT2.BED` | BED400 | 6976 | `f14a2a742989b86d727615b805d73d35b92cfa2e98e4f1f2715b33c7850086aa` |
| `data_fr.cpk::battle/boss/e20B.bin::BED/e20B_END.BED` | BED400 | 20400 | `fd46f5973ff92fc5d781a282fb094494df3e8a55809730bd91d623b8b55180d8` |
| `data_fr.cpk::battle/boss/e20B.bin::BED/e20B_END1.BED` | BED400 | 52608 | `8069c2bcc904b6fa9ed727768508d39a56ecc95f3ce509fbb4d92d5c9c1073fb` |
| `data_fr.cpk::battle/boss/e20B.bin::BED/e20B_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_fr.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20B.bin::BED/e20B_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_fr.cpk::battle/boss/e20B.bin::BED/e20B_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20C.bin::BED/e20C_ENCOUNT.BED` | BED400 | 8016 | `0d7531f757e8a743070f66980adba7097a34223c7ceab5b7e15e1006db2b0f76` |
| `data_fr.cpk::battle/boss/e20C.bin::BED/e20C_END.BED` | BED400 | 23760 | `98e314cd795aabd6134413891b58afcd5b06a5fa4c2668ab5014bb7c9e19ec9f` |
| `data_fr.cpk::battle/boss/e20C.bin::BED/e20C_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_fr.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20C.bin::BED/e20C_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e20D.bin::BED/e20D_ENCOUNT.BED` | BED400 | 6592 | `d23955927d2d12aa710e56458ce8d64c9b172cedcea6c36cd99c5696fb1d049f` |
| `data_fr.cpk::battle/boss/e20D.bin::BED/e20D_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data_fr.cpk::battle/boss/e20D.bin::BED/e20D_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_fr.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_fr.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_fr.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_fr.cpk::battle/boss/e20D.bin::BED/e20D_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e214.bin::BED/e214_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e214.bin::BED/e214_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e214.bin::BED/e214_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e214.bin::BED/e214_ENCOUNT.BED` | BED400 | 10176 | `b56f07d78e20e9c4dbd1adc3f9c582035dd213c4a751d4a6df5889ddd7492a3d` |
| `data_fr.cpk::battle/boss/e214.bin::BED/e214_END.BED` | BED400 | 5696 | `d109a2a14e0ee8b4fd4764e6c08aa8b78cb68e76651a42ac48c3638a0361576a` |
| `data_fr.cpk::battle/boss/e214.bin::BED/e214_HATUDO.BED` | BED400 | 3504 | `b1792d9bd7867be710d47f27d81220d79495ec12d027191ddcd917f963e0ead6` |
| `data_fr.cpk::battle/boss/e214.bin::BED/e214_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e214.bin::BED/e214_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e214.bin::BED/e214_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e214.bin::BED/e214_H_SP.BED` | BED400 | 3696 | `122de1e6d1504bf72f5f0fb267dc05aa3e80483c32cf8db7e8330feb9cfcce82` |
| `data_fr.cpk::battle/boss/e214.bin::BED/e214_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e215.bin::BED/e215_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e215.bin::BED/e215_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e215.bin::BED/e215_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e215.bin::BED/e215_ENCOUNT.BED` | BED400 | 11296 | `e99453fa2913f5bbcad4d7c7b3ac29df97d5d35f5c84188af83b20bc5f4b35cf` |
| `data_fr.cpk::battle/boss/e215.bin::BED/e215_END.BED` | BED400 | 1230864 | `c30afdd4e4cf82c8eebe4aa26faf5893a3427e5bd1595dd635f9aa63e89c9fc5` |
| `data_fr.cpk::battle/boss/e215.bin::BED/e215_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_fr.cpk::battle/boss/e215.bin::BED/e215_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e215.bin::BED/e215_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e215.bin::BED/e215_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e215.bin::BED/e215_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_fr.cpk::battle/boss/e215.bin::BED/e215_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e216.bin::BED/e216_ENCOUNT.BED` | BED400 | 11200 | `1dcc41a7929352eb50ba07e44f68d24ed1c0a12e8cbe954e7cb75f735f1da34e` |
| `data_fr.cpk::battle/boss/e216.bin::BED/e216_END.BED` | BED400 | 10576 | `49b1c46532ed9d3cc5a87e47736efd0ea4ec0584910f0d09d6b57ad1153d3292` |
| `data_fr.cpk::battle/boss/e216.bin::BED/e216_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_fr.cpk::battle/boss/e216.bin::BED/e216_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e216.bin::BED/e216_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e216.bin::BED/e216_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e216.bin::BED/e216_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_fr.cpk::battle/boss/e216.bin::BED/e216_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/boss/e217.bin::BED/e217_ENCOUNT.BED` | BED400 | 6304 | `959dca87cf9853542ee07ada31ffde9a842967b3194b9a38fec7e0e912fac55c` |
| `data_fr.cpk::battle/boss/e217.bin::BED/e217_END.BED` | BED400 | 60592 | `ea818bdc095b1797f4927266ae081698520a0b3e4b3f4c4d33693bfd57d28f82` |
| `data_fr.cpk::battle/boss/e217.bin::BED/e217_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_fr.cpk::battle/boss/e217.bin::BED/e217_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_fr.cpk::battle/boss/e217.bin::BED/e217_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_fr.cpk::battle/boss/e217.bin::BED/e217_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_fr.cpk::battle/boss/e217.bin::BED/e217_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_fr.cpk::battle/result/shuffle.arc::SHUFFLE01.EPL` | EPL | 6368 | `bb7e2d674c4b18778f6f7b02304159ee7a192b90f38406353bd32754207244b5` |
| `data_fr.cpk::battle/result/shuffle.arc::SHUFF_01.EPL` | EPL | 8864 | `59627e77df24e98b1aec5cf22a9c47495e145505c6fea06b5b0429b3427a7fad` |
| `data_fr.cpk::battle/result/shuffle.arc::SHUFF_02.EPL` | EPL | 9760 | `58c0739b468638264c6d3534bd2b2c6f5af07a3e6d878fc4104351a4f01b9dad` |
| `data_fr.cpk::battle/result/shuffle.arc::SHUFF_03.EPL` | EPL | 14688 | `c50a3a6c14cb67013b24aa7f12833c401ef5ef7f5bb823ec4a45916b1d996782` |
| `data_fr.cpk::battle/result/shuffle.arc::SHUFF_04.EPL` | EPL | 9568 | `2ac489ffa035960b485696279cdcdc702e512321dfec3c5de3e0533354ac27ef` |
| `data_fr.cpk::battle/result/shuffle.arc::SHUFF_05.EPL` | EPL | 10144 | `1c02f0750e4a6e99932d04af21a9752ae50b80597201fbc7ce944bdde2b45432` |
| `data_fr.cpk::battle/result/shuffle.arc::SHUFF_06.EPL` | EPL | 15104 | `a7756c94b0471c8bedc1447c499df3233be21f8992655205858fb1e97e7d7c96` |
| `data_fr.cpk::battle/result/shuffle.arc::SHUFF_09.EPL` | EPL | 1600 | `7db6177b81d30997b063517caf3b0a1a1781a7363f3219940aaa1d3deef01bb4` |
| `data_fr.cpk::event/fishing.bin::FLOAT_01.EPL` | EPL | 7232 | `3f7721ea180386dc79379572ea567e7521b2a470d9f76167aa71bbdebdb3f5d0` |
| `data_fr.cpk::event/fishing.bin::FLOAT_WV.EPL` | EPL | 7264 | `db0358e93caaab0ccfe666c308ece5ff9acc4522ae95044fbc00be249dadb346` |
| `data_fr.cpk::event/fishing.bin::SPRAY_01.EPL` | EPL | 11424 | `6229b4e3eae59c7f62ee785737578b3aba127205f979dd528d31222cb60d0be3` |
| `data_fr.cpk::event/fishing.bin::SPRAY_02.EPL` | EPL | 4288 | `54cabd45d19febb6d5decaeb2ee9c081f8c34a1e3273c2db20449cc0fb637aa1` |
| `data_fr.cpk::event/fishing.bin::SPRAY_03.EPL` | EPL | 23296 | `34836ad2132b20984fab062cf62efbae171cd5438d7d64d11d3dbe06661d15f3` |
| `data_fr.cpk::event/fishing.bin::SPRAY_04.EPL` | EPL | 22880 | `9d383ff669c401e9ff458d0904761040d95cfbd90b64b6aef2bf04dded321238` |
| `data_fr.cpk::event/fishing.bin::WAVE_01.EPL` | EPL | 512 | `e73f641168a4d70f2cfcb10cf9d4bbb71fcf1466208de78c682d22e8ba876bb3` |
| `data_fr.cpk::event/fishing.bin::WAVE_02.EPL` | EPL | 512 | `e623b56cce128bbc917f276a6238991b9417971c74de892194acc4e17610a982` |
| `data_fr.cpk::event/fishing.bin::WAVE_03.EPL` | EPL | 864 | `7431335642b1c77dc423964534743a10b1c9193279f2e36078cba7315e7a8fd4` |
| `data_fr.cpk::event/fishing.bin::WAVE_04.EPL` | EPL | 82368 | `5975b6978e545c03916310565458748458e384874ecd227887f43dc0eb331c5f` |
| `data_fr.cpk::event/fishing.bin::WAVE_05.EPL` | EPL | 14752 | `362d3e3b9d86e96cc408b2ac392838551848ecb27d829fe2996d1581b7132cfc` |
| `data_fr.cpk::event/fishing.bin::WAVE_06.EPL` | EPL | 15232 | `7f2fc8f2c4b6baf6b48cecabdfb1ab1c09b3971cab53ea9933f4df36846d71f9` |
| `data_fr.cpk::event_data/community.bin::CARD_30.EPL` | EPL | 267120 | `51a878d3a3d380b26abeab633ad5e075894c519e3fa3436cce36af0d81cc6c91` |
| `data_fr.cpk::event_data/community.bin::CARD_EF04.EPL` | EPL | 153600 | `feb474459c1fb4b4d3b11ea541d82ea2673a32a14118275c43e0430a21ec9311` |
| `data_fr.cpk::event_data/community.bin::CARD_EF06.EPL` | EPL | 134112 | `cb1ed7c4957fdbd26b0081735cb9b4c0c55587eeb604c5d868c972e738011621` |
| `data_fr.cpk::event_data/community.bin::CARD_EF07.EPL` | EPL | 19440 | `2b55441af548b5b3c90bc1bad8253942d9f68fe7cfa2cdf75779d5f0dcbc86f4` |
| `data_fr.cpk::event_data/community.bin::CMY01_02.EPL` | EPL | 7184 | `4d4eee955d10ab94f0672798d119f60466cb4437c3027c45ad8d13c1cd826b73` |
| `data_fr.cpk::event_data/community.bin::CMY02_01.EPL` | EPL | 7232 | `b5561bee072cbe2532eb2829bf5c39fc91927f53fdf184e65a67b8d01afc5dfe` |
| `data_fr.cpk::event_data/community.bin::CMY04_06.EPL` | EPL | 2816 | `1d99560e14f4d5304bad68a200e428f909d7b806466b1e504b7c8cb1a5b79800` |
| `data_fr.cpk::event_data/community.bin::CMY05_05.EPL` | EPL | 13440 | `4252a6171efe301668d8d50dc48ad1d5157e8fb7ea0e10e1322c31690c4747e1` |
| `data_fr.cpk::event_data/community.bin::E_KIRA.EPL` | EPL | 12560 | `9b83128cbed2ba53cb6c6bea178e6a26ba031a32db7e39f0863e09adac8b5e9c` |
| `data_fr.cpk::event_data/community.bin::E_KIRA2.EPL` | EPL | 12560 | `cbc82cec6fd117a07c16d78d18265390c36c0ec058022e0f36fba5e944fabf85` |
| `data_fr.cpk::event_data/community.bin::P_GET3.EPL` | EPL | 2128 | `928fd9aaecd4c39732a2c2492670dac71de00a6dda594d74c2316286a5127c9d` |
| `data_fr.cpk::facility/cmbroot.arc::E_PER.EPL` | EPL | 1120 | `07b9bac782b287964bc963b576ddc16a1724ce44010e3bf9a86f6972a757a1a9` |
| `data_fr.cpk::facility/cmbroot.arc::GAT07_01.EPL` | EPL | 19840 | `367b85f1f9785cb0a1aace1573b309b9a9eecf9df2d174eef2d78cdc332c8d16` |
| `data_fr.cpk::facility/cmbroot.arc::PER3.EPL` | EPL | 640 | `a8420c4ae86d038a3f45da5fed5afb3bcc040783e1aa5372c1957916fb5bf9b8` |
| `data_fr.cpk::field/pack/fd031_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_fr.cpk::field/pack/fd031_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_fr.cpk::field/pack/fd031_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_fr.cpk::field/pack/fd031_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_fr.cpk::field/pack/fd069_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_fr.cpk::field/pack/fd069_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_fr.cpk::field/pack/fd069_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_fr.cpk::field/pack/fd069_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_fr.cpk::field/pack/fd069_003.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_fr.cpk::field/pack/fd069_003.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_fr.cpk::field/pack/fd069_004.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_fr.cpk::field/pack/fd069_004.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON1.EPL` | EPL | 35360 | `419957b5504a91136e2a6da0a9191adbb0d0fdcabec7745d7c6a0245337e1a0c` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON10.EPL` | EPL | 640 | `a459f3d6a49e217485c18351e1be0074790389f619fa11a21c41d10f817a629e` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON11.EPL` | EPL | 35360 | `a2b85ec8fd28435027137c00eb3db2613488890e426eb15950693e450ae3666e` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON12.EPL` | EPL | 13920 | `7c600dbf814ca52978843c10998ac5877290426bce5fc15b11d188ced6dc7655` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON13.EPL` | EPL | 2816 | `4c470d96fa640beb21f4b5c5f69da7278816df340150539b19a6d282b58d3745` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON14.EPL` | EPL | 3200 | `44511407c6c60dcb3fdffc2ea0b295f3e367373ad4e47fafb8f68ffbfa523742` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON15.EPL` | EPL | 3776 | `f376c7bd0d6fd7aa6614093b86b47465f5ce418e4456293015687a845851c6c0` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON16.EPL` | EPL | 8960 | `749535ddb03ac80e9c173a4e2c4e011bb445ef4c3afea446d9327356e03e53e8` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON17.EPL` | EPL | 5408 | `682ce6151e173a4e5d837be7324c60c7e5475112682d4f7734f32c2200316fa1` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON18.EPL` | EPL | 2816 | `ac6cae65586ea536872e38741e8d1b4f710faaa61209c23a54c2daf629f38528` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON19.EPL` | EPL | 10784 | `1712a4d006ad15b02ac959902c2ea753f852832ec6d94c459ca0dd92b2f31ed1` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON2.EPL` | EPL | 70560 | `d1f15c72717d7ae5b2b5b495c209f9ce6928d98bdd7a0a9e0d45e6c95ba8e12b` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON20.EPL` | EPL | 11456 | `3c36b0b747116189a73e67773c05f474611512d39db9f7a81c47ce562f40632e` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON21.EPL` | EPL | 16768 | `ca32d5e7033db22860116d6accd7983ace3ee5c840f8ef936f22566b987bca92` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON22.EPL` | EPL | 4448 | `d0d9ad27c09e15df5caccb17467b71e5b15ba39d20f2c325d4d445caa7aac979` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON23.EPL` | EPL | 7328 | `83a7d4ae49cc4d9237798ed7eb905ef31e237550537bd89f67cf5744fbf8d102` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON24.EPL` | EPL | 13440 | `91538d6b1c7770ad8a4b4a2c34556939c9c1e4f1fc1beede20f8ba87ac5d5e7a` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON25.EPL` | EPL | 52960 | `5d8963dec880d512cc7453b46e2c0187e4113a21f82a431656472a48467951e5` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON26.EPL` | EPL | 2816 | `37621fb4cae82626436d81fabdc9b4177905063ac5264703e7de25f58e06bad3` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON3.EPL` | EPL | 12928 | `c8174ba08c05c1d7c62cb57138c3487e376dcf16713d3341a6018fc413e46688` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON4.EPL` | EPL | 32928 | `2f173d83c1e9a766c6bd5f21b90812ba376f44e2c7fd4b314b3e8c3004087b64` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON5.EPL` | EPL | 10464 | `72c96b08b6fdf5fcc5aad1a3dc4961b30e063647d1c8bac2a51ec464789c7bdd` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON6.EPL` | EPL | 24064 | `9115e4344e6efa200a9b2bad425236d6c514a6f4109e1154b080140c0889cd1b` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON7.EPL` | EPL | 2816 | `dedee54a4f210c6a3e993d17ed78bbc896a6c1e97acafc1e50e21b7ceb78f149` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON8.EPL` | EPL | 5472 | `15038b74e6204b01c8f1fee6595dba30b926e4a37e052bc8de8f53b10fb77dec` |
| `data_fr.cpk::init.bin::event/icon.bin::_ICON9.EPL` | EPL | 34688 | `f4970730444758d9b1e2c20933ecdba4d9c69a92422d4722eb33caf384c0491b` |
| `data_fr.cpk::init.bin::field/F_BLUR.EPL` | EPL | 640 | `f7c0c9b76c0cc4cb11dbe0816d1d3d83ef72ccd00f33a9480acf0ecd55fcfb6c` |
| `data_fr.cpk::init.bin::field/F_ICON01.EPL` | EPL | 11856 | `bb0d1adabff9e8808efb526a1d5ed9a438e00209958956d7fd8b83d811836882` |
| `data_fr.cpk::init.bin::field/F_ICON02.EPL` | EPL | 11312 | `37486846f11641eae744873620fd47689a2b343adebd53a567287c1918007555` |
| `data_fr.cpk::init.bin::field/F_ICON03.EPL` | EPL | 11952 | `9f58f431f3147529befe48618a378f464538847caf9d1ac7ec6cf0fb2a1c21c7` |
| `data_fr.cpk::init.bin::field/F_ICON04.EPL` | EPL | 11312 | `a2b0a94439e8716ecece6426d24874563ad7d39294abd923b21c226ca890eeb7` |
| `data_fr.cpk::init.bin::field/F_ICON05.EPL` | EPL | 11312 | `24aa952f33209549dd9a2c0ed01f2bf1f9e1f3d1883fcd1bdcdca959d4c827ca` |
| `data_fr.cpk::init.bin::field/RAIN_01.EPL` | EPL | 9440 | `af7e203dbc36d37d53577a673f20b9a6b71fa5bd1b52189326d4ea476e560c5e` |
| `data_fr.cpk::init.bin::field/SNOW_01.EPL` | EPL | 640 | `4c614a3d103ad5745d5610865f9e2cdac2fc4b90c04b343d1f3ff61401c0bba1` |
| `data_fr.cpk::init.bin::field/SNOW_02.EPL` | EPL | 640 | `3f025061842c24fa748cdd09adabd7de493c95eae332e544557efd0602798454` |
| `data_fr.cpk::init.bin::field/effect/BREATH_01.EPL` | EPL | 640 | `9e5da14b05e6803b490c3f6e71ceac80965aaba7184b3ce317229ecf53617630` |
| `data_fr.cpk::init.bin::field/effect/FOOT_L.EPL` | EPL | 5728 | `e4169139debccf8f28e88e86e2b72a49e10ecfee15a61eb745fd1a1bf1f18eee` |
| `data_fr.cpk::init.bin::field/effect/FOOT_R.EPL` | EPL | 5728 | `83828867762a5b9ed87f11eb45f91284974350052e85d8993a737fc0229e12a2` |
| `data_fr.cpk::init.bin::insect/insect.bin::INSECT_01.EPL` | EPL | 9696 | `74be2671465a4e5e760c6697cd27f5e96dce151f48b488fc40262c75cda40ea7` |
| `data_fr.cpk::init_free.bin::battle/effect/SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::ATC_D_J.BED` | BED400 | 4448 | `f9ffa0abff120b243b99b3d6006697becb10735fc24f0fb798beab2a467ece7a` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::ATC_J_J.BED` | BED400 | 3616 | `b70ab3abd0d5491ba55fc2cb9ddfc918225ba0090e58e3aed9d396e628ce6dd3` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 6560 | `fa971120bb56cc962a0792d7af2a572779c23fa18769b814c5a531dc7e09f97f` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D_J.BED` | BED400 | 4448 | `f9ffa0abff120b243b99b3d6006697becb10735fc24f0fb798beab2a467ece7a` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J_J.BED` | BED400 | 3616 | `b70ab3abd0d5491ba55fc2cb9ddfc918225ba0090e58e3aed9d396e628ce6dd3` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 6560 | `fa971120bb56cc962a0792d7af2a572779c23fa18769b814c5a531dc7e09f97f` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_it.cpk::battle/boss/e200.bin::BED/e200_ENCOUNT.BED` | BED400 | 14288 | `4294f933ecf65bfba4e1b444999c903c3c47a29462602e79b53ed7a2e7ceb452` |
| `data_it.cpk::battle/boss/e200.bin::BED/e200_END.BED` | BED400 | 33088 | `931f2c31d66a9853504d6ad85b72593d9e62da352f97719a930c8bed0e3ab733` |
| `data_it.cpk::battle/boss/e200.bin::BED/e200_H160.BED` | BED400 | 9584 | `1a0d373c01cb5744cd98f6268da56e265539fe9b10a18a20d880348c77bc727a` |
| `data_it.cpk::battle/boss/e200.bin::BED/e200_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_it.cpk::battle/boss/e200.bin::BED/e200_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e200.bin::BED/e200_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e200.bin::BED/e200_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e200.bin::BED/e200_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e201.bin::BED/e201_ENCOUNT.BED` | BED400 | 10080 | `cf6ee31d878f652f19a197d9b18502f5e32e4c765131066088aa7a008feda7bb` |
| `data_it.cpk::battle/boss/e201.bin::BED/e201_END.BED` | BED400 | 30192 | `b31d78e5de0092924773b1d1b2e55a1955a77c1e2175c64ea794dba20d1c32fd` |
| `data_it.cpk::battle/boss/e201.bin::BED/e201_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_it.cpk::battle/boss/e201.bin::BED/e201_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e201.bin::BED/e201_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e201.bin::BED/e201_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e201.bin::BED/e201_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e202.bin::BED/e202_ENCOUNT.BED` | BED400 | 7344 | `5802ab6fc50884bbc4e1db4f97b92b60079582f1126c2ad60cc8e44567c4fc8c` |
| `data_it.cpk::battle/boss/e202.bin::BED/e202_END.BED` | BED400 | 30992 | `28078663ea649edae5bcd52b50f6e614bfb54f41f722d5d18654ea082fe50bc6` |
| `data_it.cpk::battle/boss/e202.bin::BED/e202_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_it.cpk::battle/boss/e202.bin::BED/e202_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e202.bin::BED/e202_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e202.bin::BED/e202_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e202.bin::BED/e202_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e203.bin::BED/e203_ENCOUNT.BED` | BED400 | 8960 | `97081976c63beb477742d9e10a2a83bf02c08fbac7c5e5a0c727b332304cf803` |
| `data_it.cpk::battle/boss/e203.bin::BED/e203_END.BED` | BED400 | 35536 | `6221812dc3626928a9ab734835fce955f56d3164995195624b7d20907e249863` |
| `data_it.cpk::battle/boss/e203.bin::BED/e203_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_it.cpk::battle/boss/e203.bin::BED/e203_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e203.bin::BED/e203_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e203.bin::BED/e203_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e203.bin::BED/e203_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_it.cpk::battle/boss/e203.bin::BED/e203_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e204.bin::BED/e204_ENCOUNT.BED` | BED400 | 9152 | `2b707e32159c57f61dda2a51f9a4d1bb13dbaff059eb0b294294771e29b2c155` |
| `data_it.cpk::battle/boss/e204.bin::BED/e204_END.BED` | BED400 | 7040 | `ff3be583ba835a33b94a87a47080141ccda1aaa9cbe5946ce80a861386da0534` |
| `data_it.cpk::battle/boss/e204.bin::BED/e204_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_it.cpk::battle/boss/e204.bin::BED/e204_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e204.bin::BED/e204_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e204.bin::BED/e204_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e204.bin::BED/e204_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_it.cpk::battle/boss/e204.bin::BED/e204_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e205.bin::BED/e205_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e205.bin::BED/e205_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e205.bin::BED/e205_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e205.bin::BED/e205_ENCOUNT.BED` | BED400 | 10336 | `2f5fad7c4101340a9f82380c62dc0341cf2218734ebcfb157e4f68be0eca017b` |
| `data_it.cpk::battle/boss/e205.bin::BED/e205_END.BED` | BED400 | 32320 | `151da6e1d4d39174971a53d45f228d2ef3e60e9513815541c9b7dc0e03187fe1` |
| `data_it.cpk::battle/boss/e205.bin::BED/e205_HATUDO.BED` | BED400 | 3696 | `18feb0b9a37f5d925e6a17a49030adb5b2280a2cb28fb6261a6dd1460507cdb0` |
| `data_it.cpk::battle/boss/e205.bin::BED/e205_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e205.bin::BED/e205_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e205.bin::BED/e205_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e205.bin::BED/e205_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_it.cpk::battle/boss/e205.bin::BED/e205_KAIJYO.BED` | BED400 | 5152 | `99bfb77d8d42d84c48c89e01526da14dd68075d99521a820c0675e0450e1d373` |
| `data_it.cpk::battle/boss/e205.bin::BED/e205_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e206.bin::BED/e206_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e206.bin::BED/e206_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e206.bin::BED/e206_ENCOUNT.BED` | BED400 | 6400 | `5b9ca9679564c387f222fcd3aefbba7ca6b0719893a33f2f14f35e1d09b064ce` |
| `data_it.cpk::battle/boss/e206.bin::BED/e206_END.BED` | BED400 | 31504 | `ee15eb497f11a707b85ad4c857527ba2732eaf4f9936fce305bbb4056de1d85d` |
| `data_it.cpk::battle/boss/e206.bin::BED/e206_H172.BED` | BED400 | 7552 | `74a7944243d6c20da666245d7ba3d0e76b957cb06136a5de79bbf03cd9c5f1bb` |
| `data_it.cpk::battle/boss/e206.bin::BED/e206_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_it.cpk::battle/boss/e206.bin::BED/e206_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e206.bin::BED/e206_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e206.bin::BED/e206_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e206.bin::BED/e206_H_SP.BED` | BED400 | 4064 | `7007fbcdd175abbf881d3ae61fe12bed93872c08931f3e6b82aed5bfc83a2dc7` |
| `data_it.cpk::battle/boss/e206.bin::BED/e206_OFF.BED` | BED400 | 8208 | `3a6e572de45506664a54fa77e2f355094be47cc77a6b603306bad35f8b940175` |
| `data_it.cpk::battle/boss/e206.bin::BED/e206_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e207.bin::BED/e207_ENCOUNT.BED` | BED400 | 6944 | `e5db68919f29386aa0f2142e7da82e62bea35af0985694fb2a2ded112255088f` |
| `data_it.cpk::battle/boss/e207.bin::BED/e207_END.BED` | BED400 | 32096 | `cc1f9d2b5dec6f8e5b3f3fa82b5710399537f04c31bb09896f4cee7a11e8d383` |
| `data_it.cpk::battle/boss/e207.bin::BED/e207_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_it.cpk::battle/boss/e207.bin::BED/e207_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e207.bin::BED/e207_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e207.bin::BED/e207_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e207.bin::BED/e207_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e208.bin::BED/e208_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e208.bin::BED/e208_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e208.bin::BED/e208_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e208.bin::BED/e208_ENCOUNT.BED` | BED400 | 14032 | `2275fd23ee2bea212640b51f317d363e17a98e93db834cba7a104090bb652b60` |
| `data_it.cpk::battle/boss/e208.bin::BED/e208_END.BED` | BED400 | 32336 | `b4a227dc5312f1dc2e4df9c6c0e5c11216ee2c24a029b1b7a209b50757dd2bde` |
| `data_it.cpk::battle/boss/e208.bin::BED/e208_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_it.cpk::battle/boss/e208.bin::BED/e208_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e208.bin::BED/e208_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e208.bin::BED/e208_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e208.bin::BED/e208_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_it.cpk::battle/boss/e208.bin::BED/e208_SK17B.BED` | BED400 | 13328 | `0d0a8231a695a1e9497d98246dfc0c544a03b95f827f3a45adb2fd3ffeb7a704` |
| `data_it.cpk::battle/boss/e208.bin::BED/e208_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e209.bin::BED/e209_ENCOUNT.BED` | BED400 | 9392 | `80719c0c13a8fde7ff0e545957665da6cffd72631c30e8525e0962927f919266` |
| `data_it.cpk::battle/boss/e209.bin::BED/e209_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data_it.cpk::battle/boss/e209.bin::BED/e209_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_it.cpk::battle/boss/e209.bin::BED/e209_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_it.cpk::battle/boss/e209.bin::BED/e209_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_it.cpk::battle/boss/e209.bin::BED/e209_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_it.cpk::battle/boss/e209.bin::BED/e209_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20A.bin::BED/e20A_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20A.bin::BED/e20A_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20A.bin::BED/e20A_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20A.bin::BED/e20A_ENCOUNT.BED` | BED400 | 7248 | `7ce81d540666b51712f628a3ace38d282202ae8035cd6c497bcba877582a0d26` |
| `data_it.cpk::battle/boss/e20A.bin::BED/e20A_END.BED` | BED400 | 30736 | `0a7fd4451e4224bbd65059a54872ed290c3a637eff66b564a19a7cafb69e82ec` |
| `data_it.cpk::battle/boss/e20A.bin::BED/e20A_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_it.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20A.bin::BED/e20A_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_it.cpk::battle/boss/e20A.bin::BED/e20A_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20B.bin::BED/e20B_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20B.bin::BED/e20B_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20B.bin::BED/e20B_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT.BED` | BED400 | 7408 | `bfbcd65ffdff97966112ba29739ca11224d5130bd9e24793232706a5d78b80c3` |
| `data_it.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT2.BED` | BED400 | 6976 | `f14a2a742989b86d727615b805d73d35b92cfa2e98e4f1f2715b33c7850086aa` |
| `data_it.cpk::battle/boss/e20B.bin::BED/e20B_END.BED` | BED400 | 20400 | `fd46f5973ff92fc5d781a282fb094494df3e8a55809730bd91d623b8b55180d8` |
| `data_it.cpk::battle/boss/e20B.bin::BED/e20B_END1.BED` | BED400 | 52608 | `8069c2bcc904b6fa9ed727768508d39a56ecc95f3ce509fbb4d92d5c9c1073fb` |
| `data_it.cpk::battle/boss/e20B.bin::BED/e20B_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_it.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20B.bin::BED/e20B_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_it.cpk::battle/boss/e20B.bin::BED/e20B_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20C.bin::BED/e20C_ENCOUNT.BED` | BED400 | 8016 | `0d7531f757e8a743070f66980adba7097a34223c7ceab5b7e15e1006db2b0f76` |
| `data_it.cpk::battle/boss/e20C.bin::BED/e20C_END.BED` | BED400 | 23760 | `98e314cd795aabd6134413891b58afcd5b06a5fa4c2668ab5014bb7c9e19ec9f` |
| `data_it.cpk::battle/boss/e20C.bin::BED/e20C_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_it.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20C.bin::BED/e20C_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e20D.bin::BED/e20D_ENCOUNT.BED` | BED400 | 6592 | `d23955927d2d12aa710e56458ce8d64c9b172cedcea6c36cd99c5696fb1d049f` |
| `data_it.cpk::battle/boss/e20D.bin::BED/e20D_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data_it.cpk::battle/boss/e20D.bin::BED/e20D_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_it.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_it.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_it.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_it.cpk::battle/boss/e20D.bin::BED/e20D_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e214.bin::BED/e214_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e214.bin::BED/e214_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e214.bin::BED/e214_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e214.bin::BED/e214_ENCOUNT.BED` | BED400 | 10176 | `b56f07d78e20e9c4dbd1adc3f9c582035dd213c4a751d4a6df5889ddd7492a3d` |
| `data_it.cpk::battle/boss/e214.bin::BED/e214_END.BED` | BED400 | 5696 | `d109a2a14e0ee8b4fd4764e6c08aa8b78cb68e76651a42ac48c3638a0361576a` |
| `data_it.cpk::battle/boss/e214.bin::BED/e214_HATUDO.BED` | BED400 | 3504 | `b1792d9bd7867be710d47f27d81220d79495ec12d027191ddcd917f963e0ead6` |
| `data_it.cpk::battle/boss/e214.bin::BED/e214_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e214.bin::BED/e214_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e214.bin::BED/e214_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e214.bin::BED/e214_H_SP.BED` | BED400 | 3696 | `122de1e6d1504bf72f5f0fb267dc05aa3e80483c32cf8db7e8330feb9cfcce82` |
| `data_it.cpk::battle/boss/e214.bin::BED/e214_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e215.bin::BED/e215_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e215.bin::BED/e215_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e215.bin::BED/e215_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e215.bin::BED/e215_ENCOUNT.BED` | BED400 | 11296 | `e99453fa2913f5bbcad4d7c7b3ac29df97d5d35f5c84188af83b20bc5f4b35cf` |
| `data_it.cpk::battle/boss/e215.bin::BED/e215_END.BED` | BED400 | 1230864 | `c30afdd4e4cf82c8eebe4aa26faf5893a3427e5bd1595dd635f9aa63e89c9fc5` |
| `data_it.cpk::battle/boss/e215.bin::BED/e215_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_it.cpk::battle/boss/e215.bin::BED/e215_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e215.bin::BED/e215_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e215.bin::BED/e215_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e215.bin::BED/e215_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_it.cpk::battle/boss/e215.bin::BED/e215_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e216.bin::BED/e216_ENCOUNT.BED` | BED400 | 11200 | `1dcc41a7929352eb50ba07e44f68d24ed1c0a12e8cbe954e7cb75f735f1da34e` |
| `data_it.cpk::battle/boss/e216.bin::BED/e216_END.BED` | BED400 | 10576 | `49b1c46532ed9d3cc5a87e47736efd0ea4ec0584910f0d09d6b57ad1153d3292` |
| `data_it.cpk::battle/boss/e216.bin::BED/e216_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_it.cpk::battle/boss/e216.bin::BED/e216_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e216.bin::BED/e216_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e216.bin::BED/e216_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e216.bin::BED/e216_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_it.cpk::battle/boss/e216.bin::BED/e216_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/boss/e217.bin::BED/e217_ENCOUNT.BED` | BED400 | 6304 | `959dca87cf9853542ee07ada31ffde9a842967b3194b9a38fec7e0e912fac55c` |
| `data_it.cpk::battle/boss/e217.bin::BED/e217_END.BED` | BED400 | 60592 | `ea818bdc095b1797f4927266ae081698520a0b3e4b3f4c4d33693bfd57d28f82` |
| `data_it.cpk::battle/boss/e217.bin::BED/e217_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_it.cpk::battle/boss/e217.bin::BED/e217_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_it.cpk::battle/boss/e217.bin::BED/e217_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_it.cpk::battle/boss/e217.bin::BED/e217_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_it.cpk::battle/boss/e217.bin::BED/e217_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_it.cpk::battle/result/shuffle.arc::SHUFFLE01.EPL` | EPL | 6368 | `bb7e2d674c4b18778f6f7b02304159ee7a192b90f38406353bd32754207244b5` |
| `data_it.cpk::battle/result/shuffle.arc::SHUFF_01.EPL` | EPL | 8864 | `59627e77df24e98b1aec5cf22a9c47495e145505c6fea06b5b0429b3427a7fad` |
| `data_it.cpk::battle/result/shuffle.arc::SHUFF_02.EPL` | EPL | 9760 | `58c0739b468638264c6d3534bd2b2c6f5af07a3e6d878fc4104351a4f01b9dad` |
| `data_it.cpk::battle/result/shuffle.arc::SHUFF_03.EPL` | EPL | 14688 | `c50a3a6c14cb67013b24aa7f12833c401ef5ef7f5bb823ec4a45916b1d996782` |
| `data_it.cpk::battle/result/shuffle.arc::SHUFF_04.EPL` | EPL | 9568 | `2ac489ffa035960b485696279cdcdc702e512321dfec3c5de3e0533354ac27ef` |
| `data_it.cpk::battle/result/shuffle.arc::SHUFF_05.EPL` | EPL | 10144 | `1c02f0750e4a6e99932d04af21a9752ae50b80597201fbc7ce944bdde2b45432` |
| `data_it.cpk::battle/result/shuffle.arc::SHUFF_06.EPL` | EPL | 15104 | `a7756c94b0471c8bedc1447c499df3233be21f8992655205858fb1e97e7d7c96` |
| `data_it.cpk::battle/result/shuffle.arc::SHUFF_09.EPL` | EPL | 1600 | `7db6177b81d30997b063517caf3b0a1a1781a7363f3219940aaa1d3deef01bb4` |
| `data_it.cpk::event/fishing.bin::FLOAT_01.EPL` | EPL | 7232 | `3f7721ea180386dc79379572ea567e7521b2a470d9f76167aa71bbdebdb3f5d0` |
| `data_it.cpk::event/fishing.bin::FLOAT_WV.EPL` | EPL | 7264 | `db0358e93caaab0ccfe666c308ece5ff9acc4522ae95044fbc00be249dadb346` |
| `data_it.cpk::event/fishing.bin::SPRAY_01.EPL` | EPL | 11424 | `6229b4e3eae59c7f62ee785737578b3aba127205f979dd528d31222cb60d0be3` |
| `data_it.cpk::event/fishing.bin::SPRAY_02.EPL` | EPL | 4288 | `54cabd45d19febb6d5decaeb2ee9c081f8c34a1e3273c2db20449cc0fb637aa1` |
| `data_it.cpk::event/fishing.bin::SPRAY_03.EPL` | EPL | 23296 | `34836ad2132b20984fab062cf62efbae171cd5438d7d64d11d3dbe06661d15f3` |
| `data_it.cpk::event/fishing.bin::SPRAY_04.EPL` | EPL | 22880 | `9d383ff669c401e9ff458d0904761040d95cfbd90b64b6aef2bf04dded321238` |
| `data_it.cpk::event/fishing.bin::WAVE_01.EPL` | EPL | 512 | `e73f641168a4d70f2cfcb10cf9d4bbb71fcf1466208de78c682d22e8ba876bb3` |
| `data_it.cpk::event/fishing.bin::WAVE_02.EPL` | EPL | 512 | `e623b56cce128bbc917f276a6238991b9417971c74de892194acc4e17610a982` |
| `data_it.cpk::event/fishing.bin::WAVE_03.EPL` | EPL | 864 | `7431335642b1c77dc423964534743a10b1c9193279f2e36078cba7315e7a8fd4` |
| `data_it.cpk::event/fishing.bin::WAVE_04.EPL` | EPL | 82368 | `5975b6978e545c03916310565458748458e384874ecd227887f43dc0eb331c5f` |
| `data_it.cpk::event/fishing.bin::WAVE_05.EPL` | EPL | 14752 | `362d3e3b9d86e96cc408b2ac392838551848ecb27d829fe2996d1581b7132cfc` |
| `data_it.cpk::event/fishing.bin::WAVE_06.EPL` | EPL | 15232 | `7f2fc8f2c4b6baf6b48cecabdfb1ab1c09b3971cab53ea9933f4df36846d71f9` |
| `data_it.cpk::event_data/community.bin::CARD_30.EPL` | EPL | 267120 | `51a878d3a3d380b26abeab633ad5e075894c519e3fa3436cce36af0d81cc6c91` |
| `data_it.cpk::event_data/community.bin::CARD_EF04.EPL` | EPL | 153600 | `feb474459c1fb4b4d3b11ea541d82ea2673a32a14118275c43e0430a21ec9311` |
| `data_it.cpk::event_data/community.bin::CARD_EF06.EPL` | EPL | 134112 | `cb1ed7c4957fdbd26b0081735cb9b4c0c55587eeb604c5d868c972e738011621` |
| `data_it.cpk::event_data/community.bin::CARD_EF07.EPL` | EPL | 19440 | `2b55441af548b5b3c90bc1bad8253942d9f68fe7cfa2cdf75779d5f0dcbc86f4` |
| `data_it.cpk::event_data/community.bin::CMY01_02.EPL` | EPL | 7184 | `4d4eee955d10ab94f0672798d119f60466cb4437c3027c45ad8d13c1cd826b73` |
| `data_it.cpk::event_data/community.bin::CMY02_01.EPL` | EPL | 7232 | `b5561bee072cbe2532eb2829bf5c39fc91927f53fdf184e65a67b8d01afc5dfe` |
| `data_it.cpk::event_data/community.bin::CMY04_06.EPL` | EPL | 2816 | `1d99560e14f4d5304bad68a200e428f909d7b806466b1e504b7c8cb1a5b79800` |
| `data_it.cpk::event_data/community.bin::CMY05_05.EPL` | EPL | 13440 | `4252a6171efe301668d8d50dc48ad1d5157e8fb7ea0e10e1322c31690c4747e1` |
| `data_it.cpk::event_data/community.bin::E_KIRA.EPL` | EPL | 12560 | `9b83128cbed2ba53cb6c6bea178e6a26ba031a32db7e39f0863e09adac8b5e9c` |
| `data_it.cpk::event_data/community.bin::E_KIRA2.EPL` | EPL | 12560 | `cbc82cec6fd117a07c16d78d18265390c36c0ec058022e0f36fba5e944fabf85` |
| `data_it.cpk::event_data/community.bin::P_GET3.EPL` | EPL | 2128 | `928fd9aaecd4c39732a2c2492670dac71de00a6dda594d74c2316286a5127c9d` |
| `data_it.cpk::facility/cmbroot.arc::E_PER.EPL` | EPL | 1120 | `07b9bac782b287964bc963b576ddc16a1724ce44010e3bf9a86f6972a757a1a9` |
| `data_it.cpk::facility/cmbroot.arc::GAT07_01.EPL` | EPL | 19840 | `367b85f1f9785cb0a1aace1573b309b9a9eecf9df2d174eef2d78cdc332c8d16` |
| `data_it.cpk::facility/cmbroot.arc::PER3.EPL` | EPL | 640 | `a8420c4ae86d038a3f45da5fed5afb3bcc040783e1aa5372c1957916fb5bf9b8` |
| `data_it.cpk::field/pack/fd031_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_it.cpk::field/pack/fd031_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_it.cpk::field/pack/fd031_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_it.cpk::field/pack/fd031_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_it.cpk::field/pack/fd069_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_it.cpk::field/pack/fd069_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_it.cpk::field/pack/fd069_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_it.cpk::field/pack/fd069_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_it.cpk::field/pack/fd069_003.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_it.cpk::field/pack/fd069_003.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_it.cpk::field/pack/fd069_004.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_it.cpk::field/pack/fd069_004.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON1.EPL` | EPL | 35360 | `419957b5504a91136e2a6da0a9191adbb0d0fdcabec7745d7c6a0245337e1a0c` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON10.EPL` | EPL | 640 | `a459f3d6a49e217485c18351e1be0074790389f619fa11a21c41d10f817a629e` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON11.EPL` | EPL | 35360 | `a2b85ec8fd28435027137c00eb3db2613488890e426eb15950693e450ae3666e` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON12.EPL` | EPL | 13920 | `7c600dbf814ca52978843c10998ac5877290426bce5fc15b11d188ced6dc7655` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON13.EPL` | EPL | 2816 | `4c470d96fa640beb21f4b5c5f69da7278816df340150539b19a6d282b58d3745` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON14.EPL` | EPL | 3200 | `44511407c6c60dcb3fdffc2ea0b295f3e367373ad4e47fafb8f68ffbfa523742` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON15.EPL` | EPL | 3776 | `f376c7bd0d6fd7aa6614093b86b47465f5ce418e4456293015687a845851c6c0` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON16.EPL` | EPL | 8960 | `749535ddb03ac80e9c173a4e2c4e011bb445ef4c3afea446d9327356e03e53e8` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON17.EPL` | EPL | 5408 | `682ce6151e173a4e5d837be7324c60c7e5475112682d4f7734f32c2200316fa1` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON18.EPL` | EPL | 2816 | `ac6cae65586ea536872e38741e8d1b4f710faaa61209c23a54c2daf629f38528` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON19.EPL` | EPL | 10784 | `1712a4d006ad15b02ac959902c2ea753f852832ec6d94c459ca0dd92b2f31ed1` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON2.EPL` | EPL | 70560 | `d1f15c72717d7ae5b2b5b495c209f9ce6928d98bdd7a0a9e0d45e6c95ba8e12b` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON20.EPL` | EPL | 11456 | `3c36b0b747116189a73e67773c05f474611512d39db9f7a81c47ce562f40632e` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON21.EPL` | EPL | 16768 | `ca32d5e7033db22860116d6accd7983ace3ee5c840f8ef936f22566b987bca92` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON22.EPL` | EPL | 4448 | `d0d9ad27c09e15df5caccb17467b71e5b15ba39d20f2c325d4d445caa7aac979` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON23.EPL` | EPL | 7328 | `83a7d4ae49cc4d9237798ed7eb905ef31e237550537bd89f67cf5744fbf8d102` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON24.EPL` | EPL | 13440 | `91538d6b1c7770ad8a4b4a2c34556939c9c1e4f1fc1beede20f8ba87ac5d5e7a` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON25.EPL` | EPL | 52960 | `5d8963dec880d512cc7453b46e2c0187e4113a21f82a431656472a48467951e5` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON26.EPL` | EPL | 2816 | `37621fb4cae82626436d81fabdc9b4177905063ac5264703e7de25f58e06bad3` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON3.EPL` | EPL | 12928 | `c8174ba08c05c1d7c62cb57138c3487e376dcf16713d3341a6018fc413e46688` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON4.EPL` | EPL | 32928 | `2f173d83c1e9a766c6bd5f21b90812ba376f44e2c7fd4b314b3e8c3004087b64` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON5.EPL` | EPL | 10464 | `72c96b08b6fdf5fcc5aad1a3dc4961b30e063647d1c8bac2a51ec464789c7bdd` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON6.EPL` | EPL | 24064 | `9115e4344e6efa200a9b2bad425236d6c514a6f4109e1154b080140c0889cd1b` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON7.EPL` | EPL | 2816 | `dedee54a4f210c6a3e993d17ed78bbc896a6c1e97acafc1e50e21b7ceb78f149` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON8.EPL` | EPL | 5472 | `15038b74e6204b01c8f1fee6595dba30b926e4a37e052bc8de8f53b10fb77dec` |
| `data_it.cpk::init.bin::event/icon.bin::_ICON9.EPL` | EPL | 34688 | `f4970730444758d9b1e2c20933ecdba4d9c69a92422d4722eb33caf384c0491b` |
| `data_it.cpk::init.bin::field/F_BLUR.EPL` | EPL | 640 | `f7c0c9b76c0cc4cb11dbe0816d1d3d83ef72ccd00f33a9480acf0ecd55fcfb6c` |
| `data_it.cpk::init.bin::field/F_ICON01.EPL` | EPL | 11856 | `bb0d1adabff9e8808efb526a1d5ed9a438e00209958956d7fd8b83d811836882` |
| `data_it.cpk::init.bin::field/F_ICON02.EPL` | EPL | 11312 | `37486846f11641eae744873620fd47689a2b343adebd53a567287c1918007555` |
| `data_it.cpk::init.bin::field/F_ICON03.EPL` | EPL | 11952 | `9f58f431f3147529befe48618a378f464538847caf9d1ac7ec6cf0fb2a1c21c7` |
| `data_it.cpk::init.bin::field/F_ICON04.EPL` | EPL | 11312 | `a2b0a94439e8716ecece6426d24874563ad7d39294abd923b21c226ca890eeb7` |
| `data_it.cpk::init.bin::field/F_ICON05.EPL` | EPL | 11312 | `24aa952f33209549dd9a2c0ed01f2bf1f9e1f3d1883fcd1bdcdca959d4c827ca` |
| `data_it.cpk::init.bin::field/RAIN_01.EPL` | EPL | 9440 | `af7e203dbc36d37d53577a673f20b9a6b71fa5bd1b52189326d4ea476e560c5e` |
| `data_it.cpk::init.bin::field/SNOW_01.EPL` | EPL | 640 | `4c614a3d103ad5745d5610865f9e2cdac2fc4b90c04b343d1f3ff61401c0bba1` |
| `data_it.cpk::init.bin::field/SNOW_02.EPL` | EPL | 640 | `3f025061842c24fa748cdd09adabd7de493c95eae332e544557efd0602798454` |
| `data_it.cpk::init.bin::field/effect/BREATH_01.EPL` | EPL | 640 | `9e5da14b05e6803b490c3f6e71ceac80965aaba7184b3ce317229ecf53617630` |
| `data_it.cpk::init.bin::field/effect/FOOT_L.EPL` | EPL | 5728 | `e4169139debccf8f28e88e86e2b72a49e10ecfee15a61eb745fd1a1bf1f18eee` |
| `data_it.cpk::init.bin::field/effect/FOOT_R.EPL` | EPL | 5728 | `83828867762a5b9ed87f11eb45f91284974350052e85d8993a737fc0229e12a2` |
| `data_it.cpk::init.bin::insect/insect.bin::INSECT_01.EPL` | EPL | 9696 | `74be2671465a4e5e760c6697cd27f5e96dce151f48b488fc40262c75cda40ea7` |
| `data_it.cpk::init_free.bin::battle/effect/SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::ATC_D_J.BED` | BED400 | 3616 | `3bf592ac7fc718895e087564fc188463a881aa5e86508fc018119cb590e479a6` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::ATC_J_J.BED` | BED400 | 4000 | `7f1f2d09975873315930f84124fa586b8022d2f2619e00c16449849c97933d6e` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 4032 | `1b016ddae87d1f28e6cfef307e5e03e66aa51198a36bf140b785ba0878192288` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::ATC_D_J.BED` | BED400 | 3616 | `3bf592ac7fc718895e087564fc188463a881aa5e86508fc018119cb590e479a6` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::ATC_J_J.BED` | BED400 | 4000 | `7f1f2d09975873315930f84124fa586b8022d2f2619e00c16449849c97933d6e` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::ATC_Z_J.BED` | BED400 | 4032 | `1b016ddae87d1f28e6cfef307e5e03e66aa51198a36bf140b785ba0878192288` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_k.cpk::battle/boss/e200.bin::BED/e200_ENCOUNT.BED` | BED400 | 14288 | `4294f933ecf65bfba4e1b444999c903c3c47a29462602e79b53ed7a2e7ceb452` |
| `data_k.cpk::battle/boss/e200.bin::BED/e200_END.BED` | BED400 | 33088 | `931f2c31d66a9853504d6ad85b72593d9e62da352f97719a930c8bed0e3ab733` |
| `data_k.cpk::battle/boss/e200.bin::BED/e200_H160.BED` | BED400 | 9584 | `1a0d373c01cb5744cd98f6268da56e265539fe9b10a18a20d880348c77bc727a` |
| `data_k.cpk::battle/boss/e200.bin::BED/e200_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_k.cpk::battle/boss/e200.bin::BED/e200_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e200.bin::BED/e200_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e200.bin::BED/e200_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e200.bin::BED/e200_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e201.bin::BED/e201_ENCOUNT.BED` | BED400 | 10080 | `cf6ee31d878f652f19a197d9b18502f5e32e4c765131066088aa7a008feda7bb` |
| `data_k.cpk::battle/boss/e201.bin::BED/e201_END.BED` | BED400 | 30192 | `b31d78e5de0092924773b1d1b2e55a1955a77c1e2175c64ea794dba20d1c32fd` |
| `data_k.cpk::battle/boss/e201.bin::BED/e201_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_k.cpk::battle/boss/e201.bin::BED/e201_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e201.bin::BED/e201_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e201.bin::BED/e201_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e201.bin::BED/e201_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e202.bin::BED/e202_ENCOUNT.BED` | BED400 | 7344 | `5802ab6fc50884bbc4e1db4f97b92b60079582f1126c2ad60cc8e44567c4fc8c` |
| `data_k.cpk::battle/boss/e202.bin::BED/e202_END.BED` | BED400 | 30992 | `28078663ea649edae5bcd52b50f6e614bfb54f41f722d5d18654ea082fe50bc6` |
| `data_k.cpk::battle/boss/e202.bin::BED/e202_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_k.cpk::battle/boss/e202.bin::BED/e202_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e202.bin::BED/e202_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e202.bin::BED/e202_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e202.bin::BED/e202_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e203.bin::BED/e203_ENCOUNT.BED` | BED400 | 8960 | `97081976c63beb477742d9e10a2a83bf02c08fbac7c5e5a0c727b332304cf803` |
| `data_k.cpk::battle/boss/e203.bin::BED/e203_END.BED` | BED400 | 35536 | `6221812dc3626928a9ab734835fce955f56d3164995195624b7d20907e249863` |
| `data_k.cpk::battle/boss/e203.bin::BED/e203_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_k.cpk::battle/boss/e203.bin::BED/e203_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e203.bin::BED/e203_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e203.bin::BED/e203_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e203.bin::BED/e203_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_k.cpk::battle/boss/e203.bin::BED/e203_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e204.bin::BED/e204_ENCOUNT.BED` | BED400 | 9152 | `2b707e32159c57f61dda2a51f9a4d1bb13dbaff059eb0b294294771e29b2c155` |
| `data_k.cpk::battle/boss/e204.bin::BED/e204_END.BED` | BED400 | 7040 | `ff3be583ba835a33b94a87a47080141ccda1aaa9cbe5946ce80a861386da0534` |
| `data_k.cpk::battle/boss/e204.bin::BED/e204_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_k.cpk::battle/boss/e204.bin::BED/e204_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e204.bin::BED/e204_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e204.bin::BED/e204_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e204.bin::BED/e204_H_SP.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_k.cpk::battle/boss/e204.bin::BED/e204_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e205.bin::BED/e205_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e205.bin::BED/e205_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e205.bin::BED/e205_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e205.bin::BED/e205_ENCOUNT.BED` | BED400 | 10336 | `2f5fad7c4101340a9f82380c62dc0341cf2218734ebcfb157e4f68be0eca017b` |
| `data_k.cpk::battle/boss/e205.bin::BED/e205_END.BED` | BED400 | 32320 | `151da6e1d4d39174971a53d45f228d2ef3e60e9513815541c9b7dc0e03187fe1` |
| `data_k.cpk::battle/boss/e205.bin::BED/e205_HATUDO.BED` | BED400 | 3696 | `18feb0b9a37f5d925e6a17a49030adb5b2280a2cb28fb6261a6dd1460507cdb0` |
| `data_k.cpk::battle/boss/e205.bin::BED/e205_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e205.bin::BED/e205_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e205.bin::BED/e205_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e205.bin::BED/e205_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_k.cpk::battle/boss/e205.bin::BED/e205_KAIJYO.BED` | BED400 | 5152 | `99bfb77d8d42d84c48c89e01526da14dd68075d99521a820c0675e0450e1d373` |
| `data_k.cpk::battle/boss/e205.bin::BED/e205_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e206.bin::BED/e206_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e206.bin::BED/e206_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e206.bin::BED/e206_ENCOUNT.BED` | BED400 | 6400 | `5b9ca9679564c387f222fcd3aefbba7ca6b0719893a33f2f14f35e1d09b064ce` |
| `data_k.cpk::battle/boss/e206.bin::BED/e206_END.BED` | BED400 | 31504 | `ee15eb497f11a707b85ad4c857527ba2732eaf4f9936fce305bbb4056de1d85d` |
| `data_k.cpk::battle/boss/e206.bin::BED/e206_H172.BED` | BED400 | 7552 | `74a7944243d6c20da666245d7ba3d0e76b957cb06136a5de79bbf03cd9c5f1bb` |
| `data_k.cpk::battle/boss/e206.bin::BED/e206_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_k.cpk::battle/boss/e206.bin::BED/e206_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e206.bin::BED/e206_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e206.bin::BED/e206_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e206.bin::BED/e206_H_SP.BED` | BED400 | 4064 | `7007fbcdd175abbf881d3ae61fe12bed93872c08931f3e6b82aed5bfc83a2dc7` |
| `data_k.cpk::battle/boss/e206.bin::BED/e206_OFF.BED` | BED400 | 8208 | `3a6e572de45506664a54fa77e2f355094be47cc77a6b603306bad35f8b940175` |
| `data_k.cpk::battle/boss/e206.bin::BED/e206_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e207.bin::BED/e207_ENCOUNT.BED` | BED400 | 6944 | `e5db68919f29386aa0f2142e7da82e62bea35af0985694fb2a2ded112255088f` |
| `data_k.cpk::battle/boss/e207.bin::BED/e207_END.BED` | BED400 | 32096 | `cc1f9d2b5dec6f8e5b3f3fa82b5710399537f04c31bb09896f4cee7a11e8d383` |
| `data_k.cpk::battle/boss/e207.bin::BED/e207_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_k.cpk::battle/boss/e207.bin::BED/e207_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e207.bin::BED/e207_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e207.bin::BED/e207_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e207.bin::BED/e207_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e208.bin::BED/e208_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e208.bin::BED/e208_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e208.bin::BED/e208_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e208.bin::BED/e208_ENCOUNT.BED` | BED400 | 14032 | `2275fd23ee2bea212640b51f317d363e17a98e93db834cba7a104090bb652b60` |
| `data_k.cpk::battle/boss/e208.bin::BED/e208_END.BED` | BED400 | 32336 | `b4a227dc5312f1dc2e4df9c6c0e5c11216ee2c24a029b1b7a209b50757dd2bde` |
| `data_k.cpk::battle/boss/e208.bin::BED/e208_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_k.cpk::battle/boss/e208.bin::BED/e208_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e208.bin::BED/e208_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e208.bin::BED/e208_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e208.bin::BED/e208_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_k.cpk::battle/boss/e208.bin::BED/e208_SK17B.BED` | BED400 | 13328 | `0d0a8231a695a1e9497d98246dfc0c544a03b95f827f3a45adb2fd3ffeb7a704` |
| `data_k.cpk::battle/boss/e208.bin::BED/e208_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e209.bin::BED/e209_ENCOUNT.BED` | BED400 | 9392 | `80719c0c13a8fde7ff0e545957665da6cffd72631c30e8525e0962927f919266` |
| `data_k.cpk::battle/boss/e209.bin::BED/e209_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data_k.cpk::battle/boss/e209.bin::BED/e209_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_k.cpk::battle/boss/e209.bin::BED/e209_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_k.cpk::battle/boss/e209.bin::BED/e209_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_k.cpk::battle/boss/e209.bin::BED/e209_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_k.cpk::battle/boss/e209.bin::BED/e209_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20A.bin::BED/e20A_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20A.bin::BED/e20A_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20A.bin::BED/e20A_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20A.bin::BED/e20A_ENCOUNT.BED` | BED400 | 7248 | `7ce81d540666b51712f628a3ace38d282202ae8035cd6c497bcba877582a0d26` |
| `data_k.cpk::battle/boss/e20A.bin::BED/e20A_END.BED` | BED400 | 30736 | `0a7fd4451e4224bbd65059a54872ed290c3a637eff66b564a19a7cafb69e82ec` |
| `data_k.cpk::battle/boss/e20A.bin::BED/e20A_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_k.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20A.bin::BED/e20A_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20A.bin::BED/e20A_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_k.cpk::battle/boss/e20A.bin::BED/e20A_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20B.bin::BED/e20B_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20B.bin::BED/e20B_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20B.bin::BED/e20B_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT.BED` | BED400 | 7408 | `bfbcd65ffdff97966112ba29739ca11224d5130bd9e24793232706a5d78b80c3` |
| `data_k.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT2.BED` | BED400 | 6976 | `f14a2a742989b86d727615b805d73d35b92cfa2e98e4f1f2715b33c7850086aa` |
| `data_k.cpk::battle/boss/e20B.bin::BED/e20B_END.BED` | BED400 | 20400 | `fd46f5973ff92fc5d781a282fb094494df3e8a55809730bd91d623b8b55180d8` |
| `data_k.cpk::battle/boss/e20B.bin::BED/e20B_END1.BED` | BED400 | 52608 | `8069c2bcc904b6fa9ed727768508d39a56ecc95f3ce509fbb4d92d5c9c1073fb` |
| `data_k.cpk::battle/boss/e20B.bin::BED/e20B_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_k.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20B.bin::BED/e20B_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20B.bin::BED/e20B_H_SP.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_k.cpk::battle/boss/e20B.bin::BED/e20B_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20C.bin::BED/e20C_ENCOUNT.BED` | BED400 | 8016 | `0d7531f757e8a743070f66980adba7097a34223c7ceab5b7e15e1006db2b0f76` |
| `data_k.cpk::battle/boss/e20C.bin::BED/e20C_END.BED` | BED400 | 23760 | `98e314cd795aabd6134413891b58afcd5b06a5fa4c2668ab5014bb7c9e19ec9f` |
| `data_k.cpk::battle/boss/e20C.bin::BED/e20C_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_k.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20C.bin::BED/e20C_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20C.bin::BED/e20C_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e20D.bin::BED/e20D_ENCOUNT.BED` | BED400 | 6592 | `d23955927d2d12aa710e56458ce8d64c9b172cedcea6c36cd99c5696fb1d049f` |
| `data_k.cpk::battle/boss/e20D.bin::BED/e20D_END.BED` | BED400 | 11440 | `f0f29293c062295b75996fd010804538b8899378404c2712458bcf3c64909108` |
| `data_k.cpk::battle/boss/e20D.bin::BED/e20D_HATUDO.BED` | BED400 | 4064 | `c8ae696a86155ccf1eaa119bc740384faf2e773903ae7322b850f0fc2b004948` |
| `data_k.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_k.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_k.cpk::battle/boss/e20D.bin::BED/e20D_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_k.cpk::battle/boss/e20D.bin::BED/e20D_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e214.bin::BED/e214_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e214.bin::BED/e214_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e214.bin::BED/e214_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e214.bin::BED/e214_ENCOUNT.BED` | BED400 | 10176 | `b56f07d78e20e9c4dbd1adc3f9c582035dd213c4a751d4a6df5889ddd7492a3d` |
| `data_k.cpk::battle/boss/e214.bin::BED/e214_END.BED` | BED400 | 5696 | `d109a2a14e0ee8b4fd4764e6c08aa8b78cb68e76651a42ac48c3638a0361576a` |
| `data_k.cpk::battle/boss/e214.bin::BED/e214_HATUDO.BED` | BED400 | 3504 | `b1792d9bd7867be710d47f27d81220d79495ec12d027191ddcd917f963e0ead6` |
| `data_k.cpk::battle/boss/e214.bin::BED/e214_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e214.bin::BED/e214_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e214.bin::BED/e214_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e214.bin::BED/e214_H_SP.BED` | BED400 | 3696 | `122de1e6d1504bf72f5f0fb267dc05aa3e80483c32cf8db7e8330feb9cfcce82` |
| `data_k.cpk::battle/boss/e214.bin::BED/e214_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e215.bin::BED/e215_DMY_1.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e215.bin::BED/e215_DMY_2.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e215.bin::BED/e215_DMY_3.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e215.bin::BED/e215_ENCOUNT.BED` | BED400 | 11296 | `e99453fa2913f5bbcad4d7c7b3ac29df97d5d35f5c84188af83b20bc5f4b35cf` |
| `data_k.cpk::battle/boss/e215.bin::BED/e215_END.BED` | BED400 | 1230864 | `c30afdd4e4cf82c8eebe4aa26faf5893a3427e5bd1595dd635f9aa63e89c9fc5` |
| `data_k.cpk::battle/boss/e215.bin::BED/e215_HATUDO.BED` | BED400 | 3696 | `46d9f7ceb7c30cd127e76e592ef2954183dd32fbc1b77e5e4e19f091695e99b5` |
| `data_k.cpk::battle/boss/e215.bin::BED/e215_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e215.bin::BED/e215_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e215.bin::BED/e215_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e215.bin::BED/e215_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_k.cpk::battle/boss/e215.bin::BED/e215_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e216.bin::BED/e216_ENCOUNT.BED` | BED400 | 11200 | `1dcc41a7929352eb50ba07e44f68d24ed1c0a12e8cbe954e7cb75f735f1da34e` |
| `data_k.cpk::battle/boss/e216.bin::BED/e216_END.BED` | BED400 | 10576 | `49b1c46532ed9d3cc5a87e47736efd0ea4ec0584910f0d09d6b57ad1153d3292` |
| `data_k.cpk::battle/boss/e216.bin::BED/e216_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_k.cpk::battle/boss/e216.bin::BED/e216_H_PER_H.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e216.bin::BED/e216_H_PER_P.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e216.bin::BED/e216_H_PER_S.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e216.bin::BED/e216_H_SP.BED` | BED400 | 1584 | `cbc44b3a6dfc32be724b08c98a5474b55b9c7f4957c561f0871f0c7d9ca77b42` |
| `data_k.cpk::battle/boss/e216.bin::BED/e216_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/boss/e217.bin::BED/e217_ENCOUNT.BED` | BED400 | 6304 | `959dca87cf9853542ee07ada31ffde9a842967b3194b9a38fec7e0e912fac55c` |
| `data_k.cpk::battle/boss/e217.bin::BED/e217_END.BED` | BED400 | 60592 | `ea818bdc095b1797f4927266ae081698520a0b3e4b3f4c4d33693bfd57d28f82` |
| `data_k.cpk::battle/boss/e217.bin::BED/e217_HATUDO.BED` | BED400 | 4064 | `ce5921de27278f8d504542ada77df67885ea8b0c96fed470657d05f1c19a7b3b` |
| `data_k.cpk::battle/boss/e217.bin::BED/e217_H_PER_H.BED` | BED400 | 7888 | `868c7f56abd0b6484feebe8093543af6ef79d967a5809835d1c29744297129f9` |
| `data_k.cpk::battle/boss/e217.bin::BED/e217_H_PER_P.BED` | BED400 | 7296 | `915594325f82146355d438dd79df0b4d71eb5d00ba5f26eec449ad1641b34580` |
| `data_k.cpk::battle/boss/e217.bin::BED/e217_H_PER_S.BED` | BED400 | 4864 | `69df90ee476a1e81ab3a1f00c1f21e3c8d258bd93f82892232e762184ba90c06` |
| `data_k.cpk::battle/boss/e217.bin::BED/e217_START.BED` | dummy-BED | 8 | `750cfa3d5c541b5adeb296d4750d64c002c70e6e3c17fa412406dc6c61cd1039` |
| `data_k.cpk::battle/btleff.bin::AGI_CRT.BED` | BED400 | 3136 | `1ae395a2d246f22174626397e2c3edebc798f74f6543661a5c9f98da5bafe336` |
| `data_k.cpk::battle/btleff.bin::ATC_CA.BED` | BED400 | 5472 | `af6490cf4f206d48ce67d41b1d9c91a7367c10e15c163cd15fdb097f60123ae0` |
| `data_k.cpk::battle/btleff.bin::ATC_D.BED` | BED400 | 9856 | `8fce9fa4c76ada68b103c77927bf6a702910f4cc9308fac0d4f6a45d25a162b7` |
| `data_k.cpk::battle/btleff.bin::ATC_D_J.BED` | BED400 | 3616 | `3bf592ac7fc718895e087564fc188463a881aa5e86508fc018119cb590e479a6` |
| `data_k.cpk::battle/btleff.bin::ATC_J.BED` | BED400 | 10784 | `78676a278dc8e9839ddacbf030c454f5f09f03f6a7d5e674671165aaf613c074` |
| `data_k.cpk::battle/btleff.bin::ATC_J_J.BED` | BED400 | 4000 | `7f1f2d09975873315930f84124fa586b8022d2f2619e00c16449849c97933d6e` |
| `data_k.cpk::battle/btleff.bin::ATC_Z.BED` | BED400 | 9344 | `7086d3ac0e349190b6067e6dbad7f6a08e8d2911c2c26e871bc5094c4898567d` |
| `data_k.cpk::battle/btleff.bin::ATC_Z_J.BED` | BED400 | 4032 | `1b016ddae87d1f28e6cfef307e5e03e66aa51198a36bf140b785ba0878192288` |
| `data_k.cpk::battle/btleff.bin::AT_JIMEN.BED` | BED400 | 5536 | `ddb43d2bc40516f11786209592ae6dd6fbb8e4530eb487e84104b8febaa8748d` |
| `data_k.cpk::battle/btleff.bin::AUTO_DK.BED` | BED400 | 2912 | `0c3c6cbacde3c327cd92659285c794caf6468f5615f91325268a0e4dbb09ccc2` |
| `data_k.cpk::battle/btleff.bin::AUTO_HJ.BED` | BED400 | 3232 | `0b480fb917b171d3a591b50ff45d055a7d547db8c9dbb0b9f2619f82ef87df01` |
| `data_k.cpk::battle/btleff.bin::AUTO_SA.BED` | BED400 | 3456 | `97280a493d1a9567c82907811b333e51c3b0e8c1baa0f8da8f99550c1b437e06` |
| `data_k.cpk::battle/btleff.bin::AUTO_SR.BED` | BED400 | 3360 | `cefdd2858f48a2a559703ccf0477743f4e502123c82b756c9c9827618b62ed92` |
| `data_k.cpk::battle/btleff.bin::AYATURU.BED` | BED400 | 5216 | `8eac731e2fb007a02e940629433f97d39548b7ba49cca0b9719a6e3827a90954` |
| `data_k.cpk::battle/btleff.bin::BST_KH.BED` | BED400 | 2848 | `daf58929c0312e988bb652da9fe61f3410ec28d6cab5930f5fd0f8aa94e3472f` |
| `data_k.cpk::battle/btleff.bin::BUH_CRT.BED` | BED400 | 5600 | `dd7259ae8a0a82705a309faf9515b98ac1e48f5740e165a4aa7ea45d926e5cb5` |
| `data_k.cpk::battle/btleff.bin::CLOSE.BED` | BED400 | 4384 | `9b474e3e016b5e03e60ad3f87c11df038794f5b5e08de4fdab6c8efd1c30f6f1` |
| `data_k.cpk::battle/btleff.bin::COMB_F.BED` | BED400 | 6656 | `6c7d3f024b0b0c970f427af9134ac36c85d9bfba922c26cf893d26c563a348a6` |
| `data_k.cpk::battle/btleff.bin::DEATH.BED` | BED400 | 3200 | `9a958282b9073c023d30b2fce2c3f2a9293571c5530a49c089a9f54bef897183` |
| `data_k.cpk::battle/btleff.bin::DOWN_H.BED` | BED400 | 4384 | `a65a39cecfe79a4e3d6f628c1dd70b2fcd71f15e940f8f293c6010327ccd7e3c` |
| `data_k.cpk::battle/btleff.bin::DOWN_J.BED` | BED400 | 5664 | `84384b2d22bb53ad8b72bf64d7769f8e00742563b43ccf8ab2e2961853dabd84` |
| `data_k.cpk::battle/btleff.bin::DOWN_S.BED` | BED400 | 4896 | `f2a3ba2255396b24bfa0a7f9d30cd8de4004fe0530c3d94efe5504ff18ebdc33` |
| `data_k.cpk::battle/btleff.bin::DOWN_Y.BED` | BED400 | 5152 | `222f865ffb904e2bf174be77d85146ac5b41fca5b65685fa1a34f985f5d9327d` |
| `data_k.cpk::battle/btleff.bin::GAL_CRT.BED` | BED400 | 4736 | `3295f9593314c5591374a68e4a95851c577dc8164a499f374f7ac95e4d1a7fd2` |
| `data_k.cpk::battle/btleff.bin::GEKIKOU.BED` | BED400 | 4384 | `14bbd08f37eb6faad2598f3b83a590907bbe49e16f186fbaa1d56668d59f3b08` |
| `data_k.cpk::battle/btleff.bin::HANSYA.BED` | BED400 | 16352 | `53ae6478c20a1b15e53565901e90395c04ca6b92d8cc0674d523adc6c3eb4d18` |
| `data_k.cpk::battle/btleff.bin::HATUDO_E.BED` | BED400 | 4064 | `c94308d7aaeeead1fd1f77b0b343e492ec7ce1dbf3975646f395a41707621b29` |
| `data_k.cpk::battle/btleff.bin::HATUDO_H.BED` | BED400 | 6880 | `81397c105eb89a250fba6f0c9112f88ceb7439953ad2ca71ed644632b2269348` |
| `data_k.cpk::battle/btleff.bin::HATUDO_P.BED` | BED400 | 7296 | `751513f84144352e39411a619f47c0ea5c7bd0954a8fbe1c6f47df014572e014` |
| `data_k.cpk::battle/btleff.bin::HATUDO_S.BED` | BED400 | 5376 | `fcb0b05e03b2bce6302e4aa3885e0836b2d1739f6ac1c7ad428b3befd7f1b253` |
| `data_k.cpk::battle/btleff.bin::ITEM.BED` | BED400 | 3520 | `e2fe41c962c1a08d0f519f9f85f5f2c02ffaf0495fbd45dad910492b3b2dde30` |
| `data_k.cpk::battle/btleff.bin::KIZETU.BED` | BED400 | 7488 | `2d69580144752b395f1361dfbc9798711c96e30c6967a6735873985db02dfda1` |
| `data_k.cpk::battle/btleff.bin::KYOUHU.BED` | BED400 | 4384 | `d73b347d91485f3e1b023093567e27749decfc7ee9477ffdb521df7c5bfb074c` |
| `data_k.cpk::battle/btleff.bin::KYUSYU.BED` | BED400 | 10304 | `650969c19565418e23e9ea3e01981b0cd620bc7042508874da608c09f6403b8e` |
| `data_k.cpk::battle/btleff.bin::PANIC.BED` | BED400 | 4480 | `c2d31edac7b3b2e8e57e32d51b572a652ed42f22894131a8cb338ddb3b0b1136` |
| `data_k.cpk::battle/btleff.bin::PER.BED` | BED400 | 2240 | `dfbee56125b2ec9e2324238fbf55419b3ae9a0fe6d570f77513d085536c76e16` |
| `data_k.cpk::battle/btleff.bin::POISON.BED` | BED400 | 4384 | `758945e83b4d3dff2739491a8a9a025849ce2fb1c744f7a7c20ebf8eafdb44a6` |
| `data_k.cpk::battle/btleff.bin::ROUKA.BED` | BED400 | 4384 | `bc45e9e5372ca1268e3d153a200c254bbd1a76f7c140b5d8ee3bc20f0b5aecf1` |
| `data_k.cpk::battle/btleff.bin::R_BLOCK.BED` | BED400 | 128448 | `8201b318b89d76c5a67954d5c65af81838aec40045d9ac4a1d6c8633836bcb86` |
| `data_k.cpk::battle/btleff.bin::SHADOW_D.BED` | BED400 | 6752 | `d954e980d4ad22a2a350cf055f8172f5739e1d6efdf21f6df3c93d376971484a` |
| `data_k.cpk::battle/btleff.bin::SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_k.cpk::battle/btleff.bin::SLIP.BED` | BED400 | 7392 | `efdea12047ad1b0d1086ef612f2dafea83f333851fae04ee8c1bfd5a4ff41298` |
| `data_k.cpk::battle/btleff.bin::SPOT.BED` | BED400 | 8768 | `5db5ccf2927ad19d7ec301d22538843f9f3da6eb8809a379511d860ba946c7c2` |
| `data_k.cpk::battle/btleff.bin::SP_ATTK.BED` | BED400 | 7648 | `a31eb75116d5f070117b893ae669ba6a746dd7da81f9650839bb2e95a5c5b31e` |
| `data_k.cpk::battle/btleff.bin::SUIZYAKU.BED` | BED400 | 4384 | `ede66310abff2b5890bc0adc24b4102ab4b687e37f8e79228cc57d905173afd0` |
| `data_k.cpk::battle/btleff.bin::SYUCYU.BED` | BED400 | 5152 | `ef58293156ee717b02672cc12e18b45e1e5c8deb243734f0c74fe4f9a7635cf9` |
| `data_k.cpk::battle/btleff.bin::TUIGEKI.BED` | BED400 | 2528 | `3c244a4ea329e7d933e2369f2528f87aabc4a5b882220972b2e83e5aefbde249` |
| `data_k.cpk::battle/btleff.bin::TUKEKAE.BED` | BED400 | 4064 | `f50d15b64014745b284e18a4144abc1d75d911effedd7153b533a5b92f0f3d6b` |
| `data_k.cpk::battle/btleff.bin::ZIO_CRT.BED` | BED400 | 7808 | `777a117e709a560677df0d6352015f00d13693ad54709c981ae5ba1bd959dfbf` |
| `data_k.cpk::battle/cutin/syoukan/SYKN_1.EPL` | EPL | 144336 | `239284c2cf9a89d0c2a999ec68bdce5c9308d86b190788d1bfda5cab60b0357a` |
| `data_k.cpk::battle/cutin/syoukan/SYKN_2.EPL` | EPL | 144432 | `587a08a53a11914c9f57b66e1e3ce3c18c702fa25f6c2ff353c37dd3475102cf` |
| `data_k.cpk::battle/cutin/syoukan/SYKN_3.EPL` | EPL | 144624 | `fc168c4d0ea473f34836046303ddc4dcc607506589562167351cccf8e14a4cac` |
| `data_k.cpk::battle/cutin/syoukan/SYKN_4.EPL` | EPL | 144848 | `8d2f9e2d2cda7d0a3c80881f13ff314f7d7ad230098774e149d2537dea71cb2d` |
| `data_k.cpk::battle/cutin/syoukan/SYKN_6.EPL` | EPL | 144400 | `92f12559a9c35acf9866470e0149e589bcdd8ff07a45b955417257ef09c8ccdd` |
| `data_k.cpk::battle/cutin/syoukan/SYKN_7.EPL` | EPL | 143472 | `1b8d6dc66b37bc2cc4a3df833437d8504ad7fcbf5743c82226714fec1724b38e` |
| `data_k.cpk::battle/cutin/syoukan/SYKN_8.EPL` | EPL | 140368 | `db002707c3eae585ec646b4961777e156256f7bd403767c948b893b55a5468a3` |
| `data_k.cpk::battle/cutin/syoukan/SYKN_E109.EPL` | EPL | 173104 | `f4dab02ac23cefa1e64bc0130fdb3dea0aaf19b99d7470b9e36f45e24055ee4f` |
| `data_k.cpk::battle/effect/ATC_D_J.BED` | BED400 | 3600 | `de46659ae68138cdfbf18639a633f1d89876cd0b62134391e32704355a5c9b3a` |
| `data_k.cpk::battle/effect/ATC_J_J.BED` | BED400 | 3984 | `6c09e202f753d1fb7815b9bb7fd67d446d43ef6dac90f1a51ef9b8c845a485ad` |
| `data_k.cpk::battle/effect/ATC_Z_J.BED` | BED400 | 4016 | `9c1ff7dc8bcc0fbc0d9b5ef09721742f93d0548b18ab760e76721016486a6a32` |
| `data_k.cpk::battle/result/shuffle.arc::SHUFFLE01.EPL` | EPL | 6368 | `bb7e2d674c4b18778f6f7b02304159ee7a192b90f38406353bd32754207244b5` |
| `data_k.cpk::battle/result/shuffle.arc::SHUFF_01.EPL` | EPL | 8864 | `59627e77df24e98b1aec5cf22a9c47495e145505c6fea06b5b0429b3427a7fad` |
| `data_k.cpk::battle/result/shuffle.arc::SHUFF_02.EPL` | EPL | 9760 | `58c0739b468638264c6d3534bd2b2c6f5af07a3e6d878fc4104351a4f01b9dad` |
| `data_k.cpk::battle/result/shuffle.arc::SHUFF_03.EPL` | EPL | 14688 | `c50a3a6c14cb67013b24aa7f12833c401ef5ef7f5bb823ec4a45916b1d996782` |
| `data_k.cpk::battle/result/shuffle.arc::SHUFF_04.EPL` | EPL | 9568 | `2ac489ffa035960b485696279cdcdc702e512321dfec3c5de3e0533354ac27ef` |
| `data_k.cpk::battle/result/shuffle.arc::SHUFF_05.EPL` | EPL | 10144 | `1c02f0750e4a6e99932d04af21a9752ae50b80597201fbc7ce944bdde2b45432` |
| `data_k.cpk::battle/result/shuffle.arc::SHUFF_06.EPL` | EPL | 15104 | `a7756c94b0471c8bedc1447c499df3233be21f8992655205858fb1e97e7d7c96` |
| `data_k.cpk::battle/result/shuffle.arc::SHUFF_09.EPL` | EPL | 1600 | `7db6177b81d30997b063517caf3b0a1a1781a7363f3219940aaa1d3deef01bb4` |
| `data_k.cpk::battle/union/U15D_A.BED` | BED400 | 229136 | `bf70bea6d255daeff4ef444131c15e45091c85bc4e6344ff1620d298283d10cc` |
| `data_k.cpk::battle/union/U15E_A.BED` | BED400 | 262672 | `b1cb3137884d0155a5b2b10fbf2fa5c0a79c808cb9499c24c862986e16fba59b` |
| `data_k.cpk::battle/union/U15F_A.BED` | BED400 | 251840 | `bd8a7a2b11b045eb1c37fe24bdb438a382444e9521e3726c09952b7121c4bf94` |
| `data_k.cpk::event/fishing.bin::FLOAT_01.EPL` | EPL | 7232 | `3f7721ea180386dc79379572ea567e7521b2a470d9f76167aa71bbdebdb3f5d0` |
| `data_k.cpk::event/fishing.bin::FLOAT_WV.EPL` | EPL | 7264 | `db0358e93caaab0ccfe666c308ece5ff9acc4522ae95044fbc00be249dadb346` |
| `data_k.cpk::event/fishing.bin::SPRAY_01.EPL` | EPL | 11424 | `6229b4e3eae59c7f62ee785737578b3aba127205f979dd528d31222cb60d0be3` |
| `data_k.cpk::event/fishing.bin::SPRAY_02.EPL` | EPL | 4288 | `54cabd45d19febb6d5decaeb2ee9c081f8c34a1e3273c2db20449cc0fb637aa1` |
| `data_k.cpk::event/fishing.bin::SPRAY_03.EPL` | EPL | 23296 | `34836ad2132b20984fab062cf62efbae171cd5438d7d64d11d3dbe06661d15f3` |
| `data_k.cpk::event/fishing.bin::SPRAY_04.EPL` | EPL | 22880 | `9d383ff669c401e9ff458d0904761040d95cfbd90b64b6aef2bf04dded321238` |
| `data_k.cpk::event/fishing.bin::WAVE_01.EPL` | EPL | 512 | `e73f641168a4d70f2cfcb10cf9d4bbb71fcf1466208de78c682d22e8ba876bb3` |
| `data_k.cpk::event/fishing.bin::WAVE_02.EPL` | EPL | 512 | `e623b56cce128bbc917f276a6238991b9417971c74de892194acc4e17610a982` |
| `data_k.cpk::event/fishing.bin::WAVE_03.EPL` | EPL | 864 | `7431335642b1c77dc423964534743a10b1c9193279f2e36078cba7315e7a8fd4` |
| `data_k.cpk::event/fishing.bin::WAVE_04.EPL` | EPL | 82368 | `5975b6978e545c03916310565458748458e384874ecd227887f43dc0eb331c5f` |
| `data_k.cpk::event/fishing.bin::WAVE_05.EPL` | EPL | 14752 | `362d3e3b9d86e96cc408b2ac392838551848ecb27d829fe2996d1581b7132cfc` |
| `data_k.cpk::event/fishing.bin::WAVE_06.EPL` | EPL | 15232 | `7f2fc8f2c4b6baf6b48cecabdfb1ab1c09b3971cab53ea9933f4df36846d71f9` |
| `data_k.cpk::event_data/community.bin::CARD_30.EPL` | EPL | 267120 | `51a878d3a3d380b26abeab633ad5e075894c519e3fa3436cce36af0d81cc6c91` |
| `data_k.cpk::event_data/community.bin::CARD_EF04.EPL` | EPL | 153600 | `feb474459c1fb4b4d3b11ea541d82ea2673a32a14118275c43e0430a21ec9311` |
| `data_k.cpk::event_data/community.bin::CARD_EF06.EPL` | EPL | 134112 | `cb1ed7c4957fdbd26b0081735cb9b4c0c55587eeb604c5d868c972e738011621` |
| `data_k.cpk::event_data/community.bin::CARD_EF07.EPL` | EPL | 19440 | `2b55441af548b5b3c90bc1bad8253942d9f68fe7cfa2cdf75779d5f0dcbc86f4` |
| `data_k.cpk::event_data/community.bin::CMY01_02.EPL` | EPL | 7184 | `4d4eee955d10ab94f0672798d119f60466cb4437c3027c45ad8d13c1cd826b73` |
| `data_k.cpk::event_data/community.bin::CMY02_01.EPL` | EPL | 7232 | `b5561bee072cbe2532eb2829bf5c39fc91927f53fdf184e65a67b8d01afc5dfe` |
| `data_k.cpk::event_data/community.bin::CMY04_06.EPL` | EPL | 2816 | `1d99560e14f4d5304bad68a200e428f909d7b806466b1e504b7c8cb1a5b79800` |
| `data_k.cpk::event_data/community.bin::CMY05_05.EPL` | EPL | 13440 | `4252a6171efe301668d8d50dc48ad1d5157e8fb7ea0e10e1322c31690c4747e1` |
| `data_k.cpk::event_data/community.bin::E_KIRA.EPL` | EPL | 12560 | `9b83128cbed2ba53cb6c6bea178e6a26ba031a32db7e39f0863e09adac8b5e9c` |
| `data_k.cpk::event_data/community.bin::E_KIRA2.EPL` | EPL | 12560 | `cbc82cec6fd117a07c16d78d18265390c36c0ec058022e0f36fba5e944fabf85` |
| `data_k.cpk::event_data/community.bin::P_GET3.EPL` | EPL | 2128 | `928fd9aaecd4c39732a2c2492670dac71de00a6dda594d74c2316286a5127c9d` |
| `data_k.cpk::facility/cmbroot.arc::E_PER.EPL` | EPL | 1120 | `07b9bac782b287964bc963b576ddc16a1724ce44010e3bf9a86f6972a757a1a9` |
| `data_k.cpk::facility/cmbroot.arc::GAT07_01.EPL` | EPL | 19840 | `367b85f1f9785cb0a1aace1573b309b9a9eecf9df2d174eef2d78cdc332c8d16` |
| `data_k.cpk::facility/cmbroot.arc::PER3.EPL` | EPL | 640 | `a8420c4ae86d038a3f45da5fed5afb3bcc040783e1aa5372c1957916fb5bf9b8` |
| `data_k.cpk::field/SNOW_old.EPL` | EPL | 640 | `ff17b8ecac6518601e522bbc9eaf963fe6b3f480e82167891c228fcf9f8a2a99` |
| `data_k.cpk::field/pack/fd031_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_k.cpk::field/pack/fd031_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_k.cpk::field/pack/fd031_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_k.cpk::field/pack/fd031_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_k.cpk::field/pack/fd069_001.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_k.cpk::field/pack/fd069_001.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_k.cpk::field/pack/fd069_002.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_k.cpk::field/pack/fd069_002.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_k.cpk::field/pack/fd069_003.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_k.cpk::field/pack/fd069_003.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_k.cpk::field/pack/fd069_004.arc::F_SNOW.EPL` | EPL | 67616 | `9f5300542f7dd2a47aeaf3e8f6b335eb189b6de2738a31172e37057ea88f38fa` |
| `data_k.cpk::field/pack/fd069_004.arc::MAGATAMA.EPL` | EPL | 81280 | `bb9975e514b4226b539849dbc87075408dabc8c99a822c831633bdc3ec6825ef` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON1.EPL` | EPL | 35360 | `419957b5504a91136e2a6da0a9191adbb0d0fdcabec7745d7c6a0245337e1a0c` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON10.EPL` | EPL | 640 | `a459f3d6a49e217485c18351e1be0074790389f619fa11a21c41d10f817a629e` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON11.EPL` | EPL | 35360 | `a2b85ec8fd28435027137c00eb3db2613488890e426eb15950693e450ae3666e` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON12.EPL` | EPL | 13920 | `7c600dbf814ca52978843c10998ac5877290426bce5fc15b11d188ced6dc7655` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON13.EPL` | EPL | 2816 | `4c470d96fa640beb21f4b5c5f69da7278816df340150539b19a6d282b58d3745` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON14.EPL` | EPL | 3200 | `44511407c6c60dcb3fdffc2ea0b295f3e367373ad4e47fafb8f68ffbfa523742` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON15.EPL` | EPL | 3776 | `f376c7bd0d6fd7aa6614093b86b47465f5ce418e4456293015687a845851c6c0` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON16.EPL` | EPL | 8960 | `749535ddb03ac80e9c173a4e2c4e011bb445ef4c3afea446d9327356e03e53e8` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON17.EPL` | EPL | 5408 | `682ce6151e173a4e5d837be7324c60c7e5475112682d4f7734f32c2200316fa1` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON18.EPL` | EPL | 2816 | `ac6cae65586ea536872e38741e8d1b4f710faaa61209c23a54c2daf629f38528` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON19.EPL` | EPL | 10784 | `1712a4d006ad15b02ac959902c2ea753f852832ec6d94c459ca0dd92b2f31ed1` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON2.EPL` | EPL | 70560 | `d1f15c72717d7ae5b2b5b495c209f9ce6928d98bdd7a0a9e0d45e6c95ba8e12b` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON20.EPL` | EPL | 11456 | `3c36b0b747116189a73e67773c05f474611512d39db9f7a81c47ce562f40632e` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON21.EPL` | EPL | 16768 | `ca32d5e7033db22860116d6accd7983ace3ee5c840f8ef936f22566b987bca92` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON22.EPL` | EPL | 4448 | `d0d9ad27c09e15df5caccb17467b71e5b15ba39d20f2c325d4d445caa7aac979` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON23.EPL` | EPL | 7328 | `83a7d4ae49cc4d9237798ed7eb905ef31e237550537bd89f67cf5744fbf8d102` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON24.EPL` | EPL | 13440 | `91538d6b1c7770ad8a4b4a2c34556939c9c1e4f1fc1beede20f8ba87ac5d5e7a` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON25.EPL` | EPL | 52960 | `5d8963dec880d512cc7453b46e2c0187e4113a21f82a431656472a48467951e5` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON26.EPL` | EPL | 2816 | `37621fb4cae82626436d81fabdc9b4177905063ac5264703e7de25f58e06bad3` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON3.EPL` | EPL | 12928 | `c8174ba08c05c1d7c62cb57138c3487e376dcf16713d3341a6018fc413e46688` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON4.EPL` | EPL | 32928 | `2f173d83c1e9a766c6bd5f21b90812ba376f44e2c7fd4b314b3e8c3004087b64` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON5.EPL` | EPL | 10464 | `72c96b08b6fdf5fcc5aad1a3dc4961b30e063647d1c8bac2a51ec464789c7bdd` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON6.EPL` | EPL | 24064 | `9115e4344e6efa200a9b2bad425236d6c514a6f4109e1154b080140c0889cd1b` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON7.EPL` | EPL | 2816 | `dedee54a4f210c6a3e993d17ed78bbc896a6c1e97acafc1e50e21b7ceb78f149` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON8.EPL` | EPL | 5472 | `15038b74e6204b01c8f1fee6595dba30b926e4a37e052bc8de8f53b10fb77dec` |
| `data_k.cpk::init.bin::event/icon.bin::_ICON9.EPL` | EPL | 34688 | `f4970730444758d9b1e2c20933ecdba4d9c69a92422d4722eb33caf384c0491b` |
| `data_k.cpk::init.bin::field/F_BLUR.EPL` | EPL | 640 | `f7c0c9b76c0cc4cb11dbe0816d1d3d83ef72ccd00f33a9480acf0ecd55fcfb6c` |
| `data_k.cpk::init.bin::field/F_ICON01.EPL` | EPL | 11856 | `bb0d1adabff9e8808efb526a1d5ed9a438e00209958956d7fd8b83d811836882` |
| `data_k.cpk::init.bin::field/F_ICON02.EPL` | EPL | 11312 | `37486846f11641eae744873620fd47689a2b343adebd53a567287c1918007555` |
| `data_k.cpk::init.bin::field/F_ICON03.EPL` | EPL | 11952 | `9f58f431f3147529befe48618a378f464538847caf9d1ac7ec6cf0fb2a1c21c7` |
| `data_k.cpk::init.bin::field/F_ICON04.EPL` | EPL | 11312 | `a2b0a94439e8716ecece6426d24874563ad7d39294abd923b21c226ca890eeb7` |
| `data_k.cpk::init.bin::field/F_ICON05.EPL` | EPL | 11312 | `24aa952f33209549dd9a2c0ed01f2bf1f9e1f3d1883fcd1bdcdca959d4c827ca` |
| `data_k.cpk::init.bin::field/RAIN_01.EPL` | EPL | 9440 | `af7e203dbc36d37d53577a673f20b9a6b71fa5bd1b52189326d4ea476e560c5e` |
| `data_k.cpk::init.bin::field/SNOW_01.EPL` | EPL | 640 | `4c614a3d103ad5745d5610865f9e2cdac2fc4b90c04b343d1f3ff61401c0bba1` |
| `data_k.cpk::init.bin::field/SNOW_02.EPL` | EPL | 640 | `3f025061842c24fa748cdd09adabd7de493c95eae332e544557efd0602798454` |
| `data_k.cpk::init.bin::field/effect/BREATH_01.EPL` | EPL | 640 | `9e5da14b05e6803b490c3f6e71ceac80965aaba7184b3ce317229ecf53617630` |
| `data_k.cpk::init.bin::field/effect/FOOT_L.EPL` | EPL | 5728 | `e4169139debccf8f28e88e86e2b72a49e10ecfee15a61eb745fd1a1bf1f18eee` |
| `data_k.cpk::init.bin::field/effect/FOOT_R.EPL` | EPL | 5728 | `83828867762a5b9ed87f11eb45f91284974350052e85d8993a737fc0229e12a2` |
| `data_k.cpk::init.bin::insect/insect.bin::INSECT_01.EPL` | EPL | 9696 | `74be2671465a4e5e760c6697cd27f5e96dce151f48b488fc40262c75cda40ea7` |
| `data_k.cpk::init_free.bin::battle/effect/SHADOW_S.BED` | BED400 | 4192 | `9e1626889c2c0290870834cc0bfd6bf13c4f5ec0a13b5ba4cd7388beb5a4227c` |
| `data_k.cpk::skill/100.pac::100.BED` | BED400 | 197152 | `7f8de83b32825b23e5781c22e9d60829b4b52caedbd2e0b12758ac513c5052a2` |
| `data_k.cpk::skill/101.pac::101.BED` | BED400 | 207296 | `6829029f035cdbd5885d78bb7bbf6093a541d82b625e0bae21aa566dce3cbc5d` |
| `data_k.cpk::skill/102.pac::102.BED` | BED400 | 212416 | `af0afb7b99a5ac689b6e60ef7870aead498ad19dc4837245284de98de14a128d` |
| `data_k.cpk::skill/11C.pac::11C.BED` | BED400 | 430656 | `28af1b802d4ff23ba100669d1e3a7d122d1d71b1c81a49f929ed04a7151864a7` |
| `data_k.cpk::skill/172.pac::172.BED` | BED400 | 109824 | `4e6dbea9b498888bdff410af3fa69cbb08ec3739448505b58ccc11248c09db7b` |
| `data_k.cpk::skill/173.pac::173.BED` | BED400 | 163968 | `eb630c1e1764258f4984267a127ba078b3f25a4c94288d5e707db3edd94a5c4c` |
| `data_k.cpk::skill/174.pac::174.BED` | BED400 | 248736 | `f78e63bb66795e019b5c0594aede8a0d6b228880eb5f163a17b32728c7e0e856` |
| `data_k.cpk::skill/175.pac::175.BED` | BED400 | 248736 | `47bf47ba13f77b4d233538fbd440a226192d8cc1a590597d3be20cc5bf06ab59` |
| `data_k.cpk::skill/176.pac::176.BED` | BED400 | 248736 | `f6285ada612c320533732a753b116f94d1e3198a2b634a9e549c2a05a7a8469c` |
| `data_k.cpk::skill/177.pac::177.BED` | BED400 | 248736 | `dcd2369c24adcada1578fdfe155b5003374bff8c930183b97fa9514ceb1a4c70` |
| `data_k.cpk::skill/18E.pac::18E.BED` | BED400 | 212416 | `6dc1a8e243a4141870da4728961efe6dcc29ebb38de9e3c66ad47d69a84e483d` |
| `data_k.cpk::skill/18F.pac::18F.BED` | BED400 | 407104 | `8962ee7112d9fffb7b5c2d660438e4bbb15d71781af7f606947fc71d99e6011a` |

## Immediate containing archives

| Archive chain | Selected children | Archive extracted SHA-256 |
|---|---:|---|
| `data.cpk` | 510 | `b9e88940eb044e813aaf097af94ecd21b21091a98bf8835c6c1505cd24ae08fb` |
| `data.cpk::battle/battle_pack.bin::btleff.bin` | 49 | `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin` | 49 | `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| `data.cpk::battle/boss/e200.bin` | 8 | `12cb61533ca5c8a24e0bef3625ae9feb33599015ee6562ffd9600bab91678533` |
| `data.cpk::battle/boss/e201.bin` | 7 | `8428d18f4b54f0ffbbd0c1eb508e155c7ebab12e5b27903e9f1f352d710a06aa` |
| `data.cpk::battle/boss/e202.bin` | 7 | `554f7db2d4ef88ec80c1b1ff6dca1f075b070af7fa37a4be865feb62c9f5f180` |
| `data.cpk::battle/boss/e203.bin` | 8 | `6d21eca12595ebd155c90971687ec63a245e06681620d83c1a62aa8ca840293a` |
| `data.cpk::battle/boss/e204.bin` | 8 | `48b65c76354c1dca4a5e3810aafb7efc1020a00aa9cd5c79f3449a4e6b5eee24` |
| `data.cpk::battle/boss/e205.bin` | 12 | `467b4892f459c3752bcb5e4dda55b371b15669bf8e478ba13acbb22429161f27` |
| `data.cpk::battle/boss/e206.bin` | 12 | `f57f766b467a51ba327a27b0393ec0df54767abc272b9c0807e012098c49316b` |
| `data.cpk::battle/boss/e207.bin` | 7 | `a4e7ebb20c4765cc49cb5390fcbc8746a3c0ad5f8caafb90114b4b1c9133c5a1` |
| `data.cpk::battle/boss/e208.bin` | 12 | `1e14e4182e677916018ac51f9852885036ab037ae12e7fc002322b0c2f391d35` |
| `data.cpk::battle/boss/e209.bin` | 7 | `12bdb74b8b5d4a7e7099d6152cb5587f6d47820a56d3a412b8a8cdb3dc315535` |
| `data.cpk::battle/boss/e20A.bin` | 11 | `5eaec65109d6796ed0598697454a499b963d01e316f2f069fe2b93d2289132e9` |
| `data.cpk::battle/boss/e20B.bin` | 13 | `4bd2f2a72e582f8d276aafac7dea488b379cc21ce02d1d28dcedb7905d93e544` |
| `data.cpk::battle/boss/e20C.bin` | 7 | `5ae243f96745a7d727c5573789b16fa81118e8d9ff6f9809db66eef18e2d2bfb` |
| `data.cpk::battle/boss/e20D.bin` | 7 | `8ff3748977d3a4a54f858094dbc1790f561bde0b4fd23c03c26049f6792818fd` |
| `data.cpk::battle/boss/e214.bin` | 11 | `e474bd135a8d37d3b6c859788cba4ff4dc4ed77a856453756dd4e70c58b14db6` |
| `data.cpk::battle/boss/e215.bin` | 11 | `8dba9895db793c0b97afd012f003c9ed24745bdc2b010322660f3a0ef64b691e` |
| `data.cpk::battle/boss/e216.bin` | 8 | `98ef5a0a001368c2a66919814b4f3b3b93ac0460461db38ea1acd379073a57fc` |
| `data.cpk::battle/boss/e217.bin` | 7 | `45132ddd67a43790cb7e8768187131324c00e8820dbf083c77ef77219a50eba2` |
| `data.cpk::battle/btleff.bin` | 49 | `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| `data.cpk::battle/result/shuffle.arc` | 8 | `69b20601b35130af0e0d6643c97974f2962ee96d1365e6289cc10f10a4707acd` |
| `data.cpk::event/fishing.bin` | 12 | `9c9e4f020a3688aef60a2c18fee1e5c11d72772a4e9c3626ac6024e73302812b` |
| `data.cpk::event_data/community.bin` | 11 | `062e62b0aac3f819809404468a7637ef6e7630e6e3f9905dfdfaa63cfc5241ab` |
| `data.cpk::facility/cmbroot.arc` | 3 | `01a43163bbcb3219c83fbce401251d1ed78a143b51c35e63cc267dcee10f0760` |
| `data.cpk::field/btl_effect/btl_eff.pac` | 8 | `2f4435d6bfdb948d5ce80efc89e96a57fa1d577bee59b184c0d8428da1d5626c` |
| `data.cpk::field/fld_effect.bin` | 3 | `32aef097ef4b4b767f9716d161fdc2dd63eea8c11d50676d9a982ff08c52b5a0` |
| `data.cpk::field/pack/dng_init.pac` | 9 | `9ae7885bbdc966dfed7c1ebcf655841181b5bad1d87cf3821925fb3916bbdf53` |
| `data.cpk::field/pack/f007_002_rmd.arc` | 3 | `b5a94e8209cdcec1badb2e35d4c16b455d5bfd609896cbe897820ac4024bfeca` |
| `data.cpk::field/pack/f011_001_rmd.arc` | 1 | `f520f63d34147d5426011cd28db12c857c4a6febb344f7826c7776b161e447a4` |
| `data.cpk::field/pack/fd031_001.arc` | 2 | `98d7ce33587b404a107c106d6ac569440306a09477ebd083856bc9fecaa54024` |
| `data.cpk::field/pack/fd031_002.arc` | 2 | `e8e182fc079cbd8d174e1450055819238671f49173c23bce55de14f30c845422` |
| `data.cpk::field/pack/fd049_000.arc` | 2 | `6f7fb64dd62c9364ee5471025845547489ad71c4388920d2daaaf83596b4c3b0` |
| `data.cpk::field/pack/fd069_001.arc` | 2 | `841f9653c5fe5eed38b2805df7f40e35a5c3f19329170496013ff185811ce7d2` |
| `data.cpk::field/pack/fd069_002.arc` | 2 | `1bc4fedf26f33d4909e42566b8f63a4964e3d0c27091466c0cf3d0c8c8c9c2ac` |
| `data.cpk::field/pack/fd069_003.arc` | 2 | `98de58a4182e2290503680f5fb2633ae8fcab676e5458caeba9c509abaa1c20f` |
| `data.cpk::field/pack/fd069_004.arc` | 2 | `c21d29f9220b7621af6a02970c99d5f07fc6f6c25649091d232cc674d6aaa3a3` |
| `data.cpk::field/pack/object.arc` | 5 | `002f38f35122b92e84b5af4e80e576c6c2cbc5e85bde0f905d7024d24be027a4` |
| `data.cpk::init.bin` | 12 | `260f3e5537cc97e87c9a7d179ddecd42be9fe26363f8d7d5c5fca0a1e2955c2b` |
| `data.cpk::init.bin::event/icon.bin` | 26 | `7799e841994b2688152ccb5a8622b104ded5903fd53cc48509d7841ce36a9b16` |
| `data.cpk::init.bin::insect/insect.bin` | 1 | `348d03c29eba568b87f1a7d4cce688a9b32ad08bc67280652111fb389ea8e883` |
| `data.cpk::init_free.bin` | 1 | `c96b612f7b0688a74dff9e4db08fddb37239d571d5cc2f0d9a51a5c310992890` |
| `data.cpk::skill/000.pac` | 1 | `489795d0c18736724f4f22ec44034308619c88dc7cbaa5ab0054a066cb8b1fa7` |
| `data.cpk::skill/001.pac` | 1 | `c2b25d5d3f17bd39310f3f6516f38a515baa41564d2f9e384ec771f14b83ab4e` |
| `data.cpk::skill/002.pac` | 1 | `8fb1911dc365af7eba3b79db33ffafae09c346ab385e056b1c75cdcf0409bd79` |
| `data.cpk::skill/003.pac` | 1 | `f90c4a8347d4fd616b78c7527c148f10fe95dd9553ab89f31f0f309dd95ca5a7` |
| `data.cpk::skill/004.pac` | 1 | `c2196f305fbf4570c46bd079aa60034afe87bfc3d052a4d6953a79b006fa7ae8` |
| `data.cpk::skill/005.pac` | 1 | `1801a18abac9a9da0a79f2edcd2e5d6c99b095b0f7b326a5bbf081e47cf7cdaf` |
| `data.cpk::skill/006.pac` | 1 | `d25f5afa7ab4a2437798f8a6d07839d29f34adc18ae4f9307b242e82d1a1e834` |
| `data.cpk::skill/007.pac` | 1 | `de6cd35a37320eafd452e9e1cccc3c790ff473b807f8b65b52b002c3bd77096f` |
| `data.cpk::skill/008.pac` | 1 | `9e38f07f7ccb08b562b9eabdbcf6f033f2565541b565b077f8984c230f57af29` |
| `data.cpk::skill/009.pac` | 1 | `4ae3463bf726f4723046f5423adb5f5f87dc88a8f973831dd008178d036afcbc` |
| `data.cpk::skill/00A.pac` | 1 | `a18304a4ad98ce1cfb9bc53f5de49d2af5a08788dedff08963d2430407d096fe` |
| `data.cpk::skill/00B.pac` | 1 | `cfb8ea876db00555a33734168900e5c158ec425a4644c46927a6785166f610d4` |
| `data.cpk::skill/00C.pac` | 1 | `14eb085681bdf2976fb14dec0438a0db1fe56da69babb03a79fec68d221ad179` |
| `data.cpk::skill/00D.pac` | 1 | `9411e2ffc7e1a5811ad19f97fd197e7cfc2deb8363a67d09e4a359a94b3e9f2b` |
| `data.cpk::skill/00E.pac` | 1 | `9bcc8590b23048b01b9294bdc020dc81595d245c8629788aa6f07de63b18a4da` |
| `data.cpk::skill/00F.pac` | 1 | `5a350888f6c49c721129ca66f46b04b3998afe6ce24ffa6b1717cb0c0a8b7636` |
| `data.cpk::skill/010.pac` | 1 | `c240bd0fa4728fdaae0676e799b559426931ffffd7cdd7e2627857213610a476` |
| `data.cpk::skill/011.pac` | 1 | `755532f0bd7125ab5751c46b5f3709a2d567753a05594ba627a5495d46d2a49d` |
| `data.cpk::skill/012.pac` | 1 | `1aa344d1c9c2bd09336501767ca9147b56c1049348694dcc9d92de224580b79b` |
| `data.cpk::skill/013.pac` | 1 | `39af39df27d46a2e1085eec706292d822574dac64fcc076b0ff2c88144db35d3` |
| `data.cpk::skill/014.pac` | 1 | `e716182a3a3a13708956ef1f0194adb953b312af427de95774324ad7b60d051e` |
| `data.cpk::skill/015.pac` | 1 | `1a8219d96cb3130657d001c49e5df7c611c15fc9da10ebde214c36d49acfc3b6` |
| `data.cpk::skill/016.pac` | 1 | `bce7a5828b9114a6262b8e9d0aa80757deb0307e1bf761b1d98e17d35f5b5b0c` |
| `data.cpk::skill/017.pac` | 1 | `538a2cd7c1bb2768fb0ba303fb0951678d9e4a11b8f922c8a677fed96e49dea5` |
| `data.cpk::skill/018.pac` | 1 | `749f911406ca1fcb16cc4a1a99c46dcf36776a83f3cb7aa5f6ea8be4a73daac5` |
| `data.cpk::skill/019.pac` | 1 | `acd4e38b03f550340c6710104ee6cb47c560cb07ebdd19ea85c04c00c6fe2f19` |
| `data.cpk::skill/01A.pac` | 1 | `baca456c95634c5edfcdb72d6fe95a8e3c22b7d837c862b418fa0cc1bb6f163f` |
| `data.cpk::skill/01B.pac` | 1 | `a1411d0ad8cb04c9dd53015627baccfe13dcd4662a47a093f7de7c5819dae423` |
| `data.cpk::skill/01C.pac` | 1 | `d6178d404c996bff2276471fe50d0c98f61077d4eaf2427bc6f1fd18df6f9337` |
| `data.cpk::skill/01D.pac` | 1 | `4cd360b402ba10efd713697fdd9e16fff7e1f98ad35a6d44b3df01c98503111a` |
| `data.cpk::skill/01E.pac` | 1 | `aab90fd67e37b2ae837ed1364bf844c4d5dafd52fe9772cc353c1b8d6d2359d7` |
| `data.cpk::skill/01F.pac` | 1 | `20a4357810ed9bda1ef1adea71a3e9fc94f015a568f176a57248fcc94f241a17` |
| `data.cpk::skill/020.pac` | 1 | `f20aabbeffa5b768dff02462563cab0ff8c85f9d1c7b54e3a8244ac10bb385db` |
| `data.cpk::skill/021.pac` | 1 | `1675c168fbf518b1910567f7611a34ae2fe0437652646303db68b6c58588e0c0` |
| `data.cpk::skill/022.pac` | 1 | `e1e0c92779d828e92563b0a2054e1c9bbfee30365ee1f3cfd872876219a208b8` |
| `data.cpk::skill/023.pac` | 1 | `475ee9c4e6271295a67283802a5ed3e8ca470339b3d36ca2c330550109362af7` |
| `data.cpk::skill/024.pac` | 1 | `e93dab82530e3c3dd046f4f31ff2e9076e8b35c1cb53f59b12fb7d2245978904` |
| `data.cpk::skill/025.pac` | 1 | `1901a3094753a35729617de8596f17bc97371e939796639a2f78fbe23e8c3c44` |
| `data.cpk::skill/026.pac` | 1 | `a13138d453f1006497659272cdd6e0424b622217fee67eb31e5b90aac530a53a` |
| `data.cpk::skill/027.pac` | 1 | `72a08b4de5bde041c671356ac91c5d6515e35232108772654e4e837a69a91339` |
| `data.cpk::skill/028.pac` | 1 | `4834963d90f5f64b62e970d20f476205c73d4c7ae161fbd018b2d93ab9c85030` |
| `data.cpk::skill/029.pac` | 1 | `bc7886406e97e75d12958f7e8d395ac85c2e696a9873bfde828c1bc7f9a78021` |
| `data.cpk::skill/02A.pac` | 1 | `87cdacafe9c24dde24ffe2744995a2e3a11155b46c815d45d641475987000a57` |
| `data.cpk::skill/02B.pac` | 1 | `733f0b2f1e8e14c45c208ca2b641d965eca47c8c061f75ec0217d03d047935d7` |
| `data.cpk::skill/02C.pac` | 1 | `57fbaf4bec81f91d460265bfb560c644a002f501d684fe397eb3a26530281dc4` |
| `data.cpk::skill/02D.pac` | 1 | `728396e551192374a6934d3d3efccb279b29ba6e220f326a6d197e21b4338703` |
| `data.cpk::skill/02E.pac` | 1 | `2c59d8cc572853d3323a15d2fb37206a03e44f0827dcac678d2df00bd0a00fd2` |
| `data.cpk::skill/02F.pac` | 1 | `472f79c347b2555872d6f979f737513fb03c6b6284d82d5deedc7b52be1b1629` |
| `data.cpk::skill/030.pac` | 1 | `ab76e65686c4d3fdb7327a6b873ffb2f37a70fedfe56abf65a744c69e29b21dc` |
| `data.cpk::skill/031.pac` | 1 | `c1e109c5efc7cecb0d64bbf5d557873d24e9834bec6e092075827eb393815a93` |
| `data.cpk::skill/032.pac` | 1 | `77454ac7796dd5f2ebc8d96f78556483da6f32f57fbfc3ccbc20f14f2f1557c9` |
| `data.cpk::skill/033.pac` | 1 | `8372e189d1952d3fca98c575681d429f693e6f4d12991e993280fb657fa17c35` |
| `data.cpk::skill/034.pac` | 1 | `31715b177f0aea17a42e794e2492e6095d554ac6c8f1aa3ec8772e9e0bf349e2` |
| `data.cpk::skill/035.pac` | 1 | `9aae80ef2361a2489c625aa51d06af10bc2d6b4c74a057450e0eee49faf7d032` |
| `data.cpk::skill/036.pac` | 1 | `279d86ce844c82d7d19cfe4ec35d52d2e5013da86d69c94099ebc2c92844cb45` |
| `data.cpk::skill/037.pac` | 1 | `28f2883fb6c41c84237e4fc74d30811ede468603d76c9f1f46ca759a51557d50` |
| `data.cpk::skill/038.pac` | 1 | `87b407ddf0a744b7a0c0eafa6340262f16b36166a9e264a728f6752b325a4449` |
| `data.cpk::skill/039.pac` | 1 | `33122a5e343776fd5db90d245a088ab819e811526a0185fc4dfc50ba916852af` |
| `data.cpk::skill/03A.pac` | 1 | `970f6e77ed482e1b7d51ec035d35b964504f8e8723794854e733da45a429aca6` |
| `data.cpk::skill/03B.pac` | 1 | `ace331892b68148ba9db78b22ac2f55e7edb5b319f95e16aca1129ebbbe9338c` |
| `data.cpk::skill/03C.pac` | 1 | `255590cc2431636ba0793db28c3182005021c910935bd3552203f7bb9a0c6657` |
| `data.cpk::skill/03D.pac` | 1 | `27de29080bac519c5f353515b88376d181743f2b23fcaa99f0d7e1e23854a87c` |
| `data.cpk::skill/03E.pac` | 1 | `9477a9161a792cc2aed3e5f9bbf458d260d6be81682452dd23282c464c53a0af` |
| `data.cpk::skill/03F.pac` | 1 | `0ec8a3e91b6c66935cad6cb7349e69716c219879a91448825bde1b27bba28fd9` |
| `data.cpk::skill/040.pac` | 1 | `769244181b65097eaf7a9af2ff94ff040ac89924bac1e983f929b0ccbf123a8b` |
| `data.cpk::skill/041.pac` | 1 | `f0a700a9dc66eca0653b6104bb394bd9d00756172f3d2d99cacd56fa72d28425` |
| `data.cpk::skill/042.pac` | 1 | `2b2f62af66d152cfd75854b7ca56c46a3f46fe2e5d2b8369bf82657e59c07006` |
| `data.cpk::skill/043.pac` | 1 | `d4892b82c859bf569143244d93b0c5ba059d0bfdbf993a9c756bec9d647c3128` |
| `data.cpk::skill/044.pac` | 1 | `9ae45914383799a275f79542c36d84100abea17f20df0d772a1b7d4df5ed59a5` |
| `data.cpk::skill/045.pac` | 1 | `54eb5f1189ba3daa84a1b185c9399fc553a07cd29d8b27b322b1d0476782afc6` |
| `data.cpk::skill/046.pac` | 1 | `c869005cfc174a33c41131413743982a5c1ecbfd461e372d3fff98c7187e5cbc` |
| `data.cpk::skill/047.pac` | 1 | `b5a673d89eb61900d8f2cd3e0aa54e1952faba419813d448e1ac8f6bc29abcf4` |
| `data.cpk::skill/048.pac` | 1 | `6ee576dc6d74122ef6dbdf4b2021d4b47488e0dbd6a78ba8a19ad495ce307fa2` |
| `data.cpk::skill/049.pac` | 1 | `6d582a6ecbd5f2aad96254673e91a22ba2fa44e40b654d52f2a7a50566b7e00e` |
| `data.cpk::skill/04A.pac` | 1 | `3460a1a40b00239be5625e8af5bb57661b1b4f6b06fcfd6c843e155a0435aedc` |
| `data.cpk::skill/04B.pac` | 1 | `342045d625780fa41b44b7eadfe7c47d317eaeea8ec8cf04be13b1c35546ad30` |
| `data.cpk::skill/04C.pac` | 1 | `cf9bea2f3834c85d31b57c70dbb17831823cf673a59ce683619232dd09ca51c7` |
| `data.cpk::skill/04D.pac` | 1 | `1ee669eb1604550f062372176a0c950c4779954e3dca91f27c42e469fa0b7e43` |
| `data.cpk::skill/04E.pac` | 1 | `d4b57a850424c0160f1baedac4250c5b7fd3090ea052412c8c985a6c5d31eb19` |
| `data.cpk::skill/04F.pac` | 1 | `0d259095e9456b05d32200864bbae40a4be9ed982f4e2e0c0fdac77268247969` |
| `data.cpk::skill/050.pac` | 1 | `dbb3275b0f5f9e07d5352ad61d1ef1adf8b693a87904bb0582b3e0e0a7089be2` |
| `data.cpk::skill/051.pac` | 1 | `3f3f7fbdd23f56272086b88f004cb77c0d676625426d578e353bca9814157232` |
| `data.cpk::skill/052.pac` | 1 | `18a3a9bd297165cfbe5d098219fea20c7dd5a0df716ea95b837cbcfdfacbb12d` |
| `data.cpk::skill/053.pac` | 1 | `25c96d7a347ccc1c0178022d8a4996e95497472be68d5ee5b852382a8dcf5a7b` |
| `data.cpk::skill/054.pac` | 1 | `5375ad8906d082b1d96245cf78c12d5de2ecb7e1916a6623064635d110efd084` |
| `data.cpk::skill/055.pac` | 1 | `a73f3ab4d610c901d525bd956120f0fcb48cec70940683ee6aed0666cb2cdf00` |
| `data.cpk::skill/056.pac` | 1 | `6ed0d006b372291831e8be5bc96dcf047eee477cc0c91b4a92bba76ab3be8b79` |
| `data.cpk::skill/057.pac` | 1 | `9dd1bf24dd07ecf60aa14f6c2c6c448f1a3e7ec34737471b6c3b65824b4f6355` |
| `data.cpk::skill/058.pac` | 1 | `6f37c606470e3b92bf552257f998ddb34352907cfb8306df0e289c7d7045d334` |
| `data.cpk::skill/059.pac` | 1 | `fa585a45a905d8c1b598f196ec9471a7cd671e8c38a8b061360eac42d2d1fed3` |
| `data.cpk::skill/05A.pac` | 1 | `ddb52208d93aa6ae3020a339a6e8bfcb09037f2127bf2badb9841b3956ee62ee` |
| `data.cpk::skill/05B.pac` | 1 | `5807727533b0bb3d4d28b3c2581ace0cc9f7490622fd53608121338baf7dee9a` |
| `data.cpk::skill/05C.pac` | 1 | `fe3c71b8ca3c46a5a3d9f47cc26c8c249c34650cd7f0bfb62f35dba295a6be01` |
| `data.cpk::skill/05D.pac` | 1 | `bd69df70d668aaf6c42604ad8a7319f7b6416b6bab37ce7d1c80f4bb128bcef9` |
| `data.cpk::skill/05E.pac` | 1 | `1cc93149db42675dbd3110700e63912702d9b7df95c94f914b0e97dd28528338` |
| `data.cpk::skill/05F.pac` | 1 | `ba527b5c1369f246bad00b2d9ca36bfef75ed879a9ddd91c1069419854fba653` |
| `data.cpk::skill/060.pac` | 1 | `9df0804a1e39ec4aaf2ac8270cfc96790b8b15f08372f56b935261c7cf492d34` |
| `data.cpk::skill/061.pac` | 1 | `47b1c25fefd065d1cb025c808382731e2eabd738a09a70e0b0a075b3dcf95de3` |
| `data.cpk::skill/062.pac` | 1 | `87be4ef4c3271bdeb3a664d14b8082a4b15c7fc7cb9eaa989a21b43c3dfad090` |
| `data.cpk::skill/063.pac` | 1 | `2de4ff47688e6139c6093223cc21fbdf1c056463d9db69392b180b229eb8ce97` |
| `data.cpk::skill/064.pac` | 1 | `371a95f8f206d6b2af872f9b9181ad513dcdacc40369a1bc3f09b78c749eca83` |
| `data.cpk::skill/065.pac` | 1 | `56890c1537617b32726adbf46807acae1a84a6c11ef1f7406d43aa08f6cc54ae` |
| `data.cpk::skill/066.pac` | 1 | `813493cc00b8d463bcba11fe1024d54471f62a54570dc0f2c65a4d8e15d8cc76` |
| `data.cpk::skill/067.pac` | 1 | `0973b4d61d6c20d4ceee53cce387fce011b6e8e6dd73f2eef5b23f6732b565f7` |
| `data.cpk::skill/068.pac` | 1 | `286ec594e6ca49a2545c8acd515aab1906938e9e11c34df969fc4c30ac86822b` |
| `data.cpk::skill/069.pac` | 1 | `1b849cedbc15c24324c4e7274d297f57f3100a6d6fd82e430877eab3a02dda89` |
| `data.cpk::skill/06A.pac` | 1 | `41b3a402f82b1c1af947c4de8cfe647e5509f29f0751f060a0b4e99636791fa2` |
| `data.cpk::skill/06B.pac` | 1 | `59563fa394f519ec3fca976a2232c50abf8b09822164a59808ec8f6254043b4b` |
| `data.cpk::skill/06C.pac` | 1 | `8b50648ecd44f681b4faf0ad7e10439c1f604f6b836443ab278cff5da65e5a5b` |
| `data.cpk::skill/06D.pac` | 1 | `89147110838cf8c8be0f51465f815700e95fd5bd451a8ecd35a6e69f54e43376` |
| `data.cpk::skill/06E.pac` | 1 | `12e119a2b5d7b647842984de7f3408f05eae191eac93c1abc8c2056d524050a5` |
| `data.cpk::skill/06F.pac` | 1 | `50ff0049f6668b1978b9de807e007e08e330fc4a0993c4ebc2e6bbb965c15999` |
| `data.cpk::skill/070.pac` | 1 | `8efafb56eff6da2cc65fe38190b3ef5357758fa4c28286c6b2419308555cb95a` |
| `data.cpk::skill/071.pac` | 1 | `64668246add2c6290e700e420b6488f5ce67cb9b70ed4fa62bc9d7c4ad912d9e` |
| `data.cpk::skill/072.pac` | 1 | `2e8c400f152f5f83c7bbd179dd801369f1c440e315b8b6bb931693276a63a8e2` |
| `data.cpk::skill/073.pac` | 1 | `7d70cc2f4b27147cf393292528294865d04cc3537979db10d89986811b15957e` |
| `data.cpk::skill/074.pac` | 1 | `644144d77ed71a682134e11f7321dbf0f71ffb460f253198c3d7699f545a4709` |
| `data.cpk::skill/075.pac` | 1 | `44c5ba8d8df9182dcf58de22e7722eab0115edd04a0d6a5a7b2dd6ae0e55c38d` |
| `data.cpk::skill/076.pac` | 1 | `ff5bd3ee6bba8324ad5319ee771f5c2f190bc69fe88aa4263c04d2c3c111ae58` |
| `data.cpk::skill/077.pac` | 1 | `10456e168167baa4ad64b8c43575545dd191aba8311249e1d71ff4c146af8318` |
| `data.cpk::skill/078.pac` | 1 | `e712f9da06d48144658df078752b6b23cd3e3b6e1ad4ffc89872f5dff749ada6` |
| `data.cpk::skill/079.pac` | 1 | `44fb9eea8fd48071e884d7ea43c3e5b169357fdb3615152b9b2e846b4c798f14` |
| `data.cpk::skill/07A.pac` | 1 | `9be9df061abfc7439dd5ee22c5c392182c7e075fe5aeba0d3e109602f5bd88a7` |
| `data.cpk::skill/07B.pac` | 1 | `ba3597713e17b887a7f418fe05af2a321ee9c44cd0b41f5ea656c8760c1789c6` |
| `data.cpk::skill/07C.pac` | 1 | `e76797218ff039b5ec7b14b7e72be4ebce2c5986e7b435928db7a680ba414d1c` |
| `data.cpk::skill/07D.pac` | 1 | `aac8836988660130591f4396003015061258924330c03184dedbf08e1e04de02` |
| `data.cpk::skill/07E.pac` | 1 | `77b6335d90f6910fa27f519be9276d13567269a3a59222b61c172812f8e2090e` |
| `data.cpk::skill/07F.pac` | 1 | `889796e7dc8f4c4756d11660434463f1a665fa6bd7080128e2e83626b05dce6e` |
| `data.cpk::skill/080.pac` | 1 | `70f742669f64dcc76ce297133c7f3d4b68bd1c98f4743137c704c9ec29e1b6f0` |
| `data.cpk::skill/081.pac` | 1 | `6a139d9125d78f494439ea2fda85b4e40f2a41c58d644213cc10c1781af069eb` |
| `data.cpk::skill/082.pac` | 1 | `9a355fb9c82d535b2afcfccdb66b2e1b9682465c2b76fe3652ded4f6a2a2e52e` |
| `data.cpk::skill/083.pac` | 1 | `d28d6444213f02ce2cef94bcd1a40d55820bfce6d0a32a8f41e217444b3036bd` |
| `data.cpk::skill/084.pac` | 1 | `d6c5ddfbac5a42c910678380bec62c6053ddd85c6274903863ff3e4fd691be48` |
| `data.cpk::skill/085.pac` | 1 | `f15a1408b841dc73b584f0a4aa52a0f083b1712edede5bdf8d734e8d5c76e214` |
| `data.cpk::skill/086.pac` | 1 | `4fe4736d4412baeb8c1bd781177a7d825ccb41d89b1650896e2e6162ca250463` |
| `data.cpk::skill/087.pac` | 1 | `ae38bce66eb7e4185d1c6493e8886d294207db8ec852454630c8b6690b0422d2` |
| `data.cpk::skill/088.pac` | 1 | `55ffdc4dd1f9163bf424703759995237f05d74d7a4c28b664fd43f246b5b11d8` |
| `data.cpk::skill/089.pac` | 1 | `77ed5192915552336855d061a5730325b280dbdd6cd4c10a66ec4814a3a65b10` |
| `data.cpk::skill/08A.pac` | 1 | `e1553133b25cccce7c62efb1aaae5476520aff9169a8e8c83b4b8140a6a27fe8` |
| `data.cpk::skill/08B.pac` | 1 | `cb2da6c6e84ea7288c2542cd969f95dbd3b3f8220b31fc912db8c495edd63822` |
| `data.cpk::skill/08C.pac` | 1 | `76f96fcc04c93d8f00a85af44cd70ed4f91710101bca03bc667f7b4abeeedcf8` |
| `data.cpk::skill/08D.pac` | 1 | `09ff8e432d0ffff6784225b7c1bbec297c1ae69eddeed4edba166fb6c0e2c2b7` |
| `data.cpk::skill/08E.pac` | 1 | `271c3544dc248fdf15757569bbd2069147fbb5903cd9cf3a85cc0eba5c9e9294` |
| `data.cpk::skill/08F.pac` | 1 | `b59a00f75ce6c474aa9c7904583aa0488e3986bd1f6d237c107cdd09f4c24e22` |
| `data.cpk::skill/090.pac` | 1 | `2a5bdef58d23a44f83afc33e1f4df62eb611d46dc53f4e54a7687b5f088d2e94` |
| `data.cpk::skill/091.pac` | 1 | `178e14501b31f6ff0209e686e00ea950d4627f47ab9f7597e7d060f2014d82ff` |
| `data.cpk::skill/092.pac` | 1 | `690a8554ac33c16e24c3cc2bdb91b2533b87f9a9d7ed56fa2f1e732c7c18f3ec` |
| `data.cpk::skill/093.pac` | 1 | `6970f8e4696d8d2736557c8d215391810eb21d73db74ce3c84c3a9429e47d470` |
| `data.cpk::skill/094.pac` | 1 | `cecac7258ceeb5124b28588b3485917b811d8049d0444d2782b439bdb94a80ee` |
| `data.cpk::skill/095.pac` | 1 | `ebf0c23abf8cbda53b9ec0a1c8a465c27cf4e9979ec5a4a50b72fca4cf332605` |
| `data.cpk::skill/096.pac` | 1 | `b94a3d6aa51b5fa1b94223d283c02bc4ce84477c9162275523f6349257557560` |
| `data.cpk::skill/097.pac` | 1 | `e4d83395fb7db8a113c325f93a946b5d034e2ea8a74b006a7dab1f81ef2050ee` |
| `data.cpk::skill/098.pac` | 1 | `9e13e092f472323d823a5ae5eda37cc3c6f90d90fd7371e310dc38563bd1aa0e` |
| `data.cpk::skill/099.pac` | 1 | `b8141cfd53a1e545a0c068b74ac75b82a83e1875d9309abd925d42b29c2a25a1` |
| `data.cpk::skill/09A.pac` | 1 | `613b9677500a4abdf61ce581781d97cb2b356380316b9e94dce34bcbdd6d2154` |
| `data.cpk::skill/09B.pac` | 1 | `17498aa13031bc315ef2dd6a252567717e8d6734313d4413fb7e552157efb5ce` |
| `data.cpk::skill/09C.pac` | 1 | `24653f2df2be7ab0d4ce99224b219466928c57ee7f6c8dccdeb143143b05b3c2` |
| `data.cpk::skill/09D.pac` | 1 | `be43edbaad64e2f6e9b10f8ce04fb3dfdd4d8ce93461c14e90b411c909d6cabd` |
| `data.cpk::skill/09E.pac` | 1 | `983a93f2dd01b8e538ac935b3d9516bb29ddf81b19324bc985802c5d7baaf787` |
| `data.cpk::skill/09F.pac` | 1 | `acb6ba97b94823a5f58455f98420bbb23a64b8f9219f8d0df513118bbb91862d` |
| `data.cpk::skill/0A0.pac` | 1 | `55441b153d9269f0f81efee9a4bb637e935f10ccd65736b92aa9bcf7bf39f82e` |
| `data.cpk::skill/0A1.pac` | 1 | `533d025cbef3e8a015bfd567f4987e74837cdcbd4558513102bc76dbab0c1213` |
| `data.cpk::skill/0A2.pac` | 1 | `8903bb0be5a658d6100446acf57f79ac4577e4d2383b0416d5c1fd7477c45536` |
| `data.cpk::skill/0A3.pac` | 1 | `ee5c7cbbe65d90761f4b192ab781514035ffae4f8af64601e6af3692b4b59438` |
| `data.cpk::skill/0A4.pac` | 1 | `ef669269665764d83f01c35fe8bad5c5ee4333a7e136ed756de27861a793f7cb` |
| `data.cpk::skill/0A5.pac` | 1 | `bfee83f77f83fdcd631ba9d7fbf5468d50d41eb2a71af804e2e69451652c4ed7` |
| `data.cpk::skill/0A6.pac` | 1 | `5dd1a3182d45b607cbf8d7ce36129d5c5f47926f54560acb108638ce3aad706d` |
| `data.cpk::skill/0A7.pac` | 1 | `ec63002897f898930509244da8c7ef3637101bd8153b109c0383afa2af6009e6` |
| `data.cpk::skill/0A8.pac` | 1 | `05098367a3f63fbd32ab4223421dc091e82739a3674badac77e22a7cb05d3d9e` |
| `data.cpk::skill/0A9.pac` | 1 | `1a2377661a6b921056f5c1c421652a25155ee0655c6c82203dcd5fdcf81a575b` |
| `data.cpk::skill/0AA.pac` | 1 | `27e374ad555cac6d0bb55e47ad8589fc5b4179ad8dd2951cacda53ff346c9ee9` |
| `data.cpk::skill/0AB.pac` | 1 | `c3d03e5a56c918736607b6eb41f6eea76a2f770bb06ecc70f8be7e278f091c47` |
| `data.cpk::skill/0AC.pac` | 1 | `61a33d9f06ebeb3af7389db39b900d74304610ea2fcc3f3ac8d5ae3d8aa1a552` |
| `data.cpk::skill/0AD.pac` | 1 | `97b85c2968ca5ee4f97090f0c728048f784e91ffe21eabbe2b1f18d6261663af` |
| `data.cpk::skill/0AE.pac` | 1 | `c0ab38d0fc4ff07a8cc88338acdfaf45175fdf0c7247cd773332e951e772837a` |
| `data.cpk::skill/0AF.pac` | 1 | `24db624e6a7a038c511c9e539056f2b228e80f494c0477f9d5307f023b38be9a` |
| `data.cpk::skill/0B0.pac` | 1 | `0da73e198d01546bb4da0311fb7ec4cb71baf45e86e1416cb8fd3a2d4439cbae` |
| `data.cpk::skill/0B1.pac` | 1 | `393391b023f0f01c6da7eb6c70a5b9cec134c90fdae41a8b644c748c74482118` |
| `data.cpk::skill/0B2.pac` | 1 | `ec3b0498bb935cfbbb1a8891f5631614a30bf88bfee6975429f2ceb48baaa2cc` |
| `data.cpk::skill/0B3.pac` | 1 | `e63e4f94c7f295ffe0946eb4e429933ca51ebe1b3a289c47251a73ff17799842` |
| `data.cpk::skill/0B4.pac` | 1 | `58f6e81c2cd1fc6caf38c14b3ea91516e803332cd3a5d9dd990bd595eb4bcfbc` |
| `data.cpk::skill/0B5.pac` | 1 | `fe3b208306c133ed4e76db36bfef3edfb7adc9abf70d61b5f5b95951c096fcb2` |
| `data.cpk::skill/0B6.pac` | 1 | `135a926df3f855e52d7f0301917eccbb106b9df7b2855597060831ace48d6339` |
| `data.cpk::skill/0B7.pac` | 1 | `57219371d93fe8699c8e28225a2fbe3dbc3d220a1a6e3e35814a57c047da667c` |
| `data.cpk::skill/0B8.pac` | 1 | `eb7508ed7a2a28e5a3392c9a01398b14dee1c5c53df09af038c104d35fdb7224` |
| `data.cpk::skill/0B9.pac` | 1 | `2ed2a8a2e78c441804da4f3f0a307fdf3cc4bcfe73b299749ff4257b16669017` |
| `data.cpk::skill/0BA.pac` | 1 | `7be0b1eab96103e635e51f4897c97e2a2396d4752ace949f589491e2f65d9efb` |
| `data.cpk::skill/0BB.pac` | 1 | `6c6fa8e3c2a4aa358b7b573851ae02ac8cbfed070ef9c197349881ac68c09800` |
| `data.cpk::skill/0BC.pac` | 1 | `078ecefa24c393eec278ef4c5fce226836deee545d49f04d4954e738f4a95cd5` |
| `data.cpk::skill/0BD.pac` | 1 | `7ac137ef1879eadc472eb4187217f802776b29c9d367b664ced45d4387f9e40b` |
| `data.cpk::skill/0BE.pac` | 1 | `c863ceb69fe088551d6994c18c3aaafbab6065c8a79ccb63528cf93dcd9ca457` |
| `data.cpk::skill/0BF.pac` | 1 | `0077090f3d67c07fa91cfe8617780e62b172af7ae21821ff82b054f8c963bda4` |
| `data.cpk::skill/0C0.pac` | 1 | `56af8370ea371c7fbcf507803ef885744bb198caf2350ed6cf0882984df707c9` |
| `data.cpk::skill/0C1.pac` | 1 | `b5191be5de5fe27f15b2087c05e47ab5f2911f32edb2707c4ee311e086b6be49` |
| `data.cpk::skill/0C2.pac` | 1 | `d67c9cdb85c0c0d8a722677d08d7fd7222e6adefc3eda110d5512fb697ce73ae` |
| `data.cpk::skill/0C3.pac` | 1 | `c034cf34353e557009879870eb4e5c6194e4f6c09a96019ad49682f7f2336dd2` |
| `data.cpk::skill/0C4.pac` | 1 | `bafe9e348fc00f30234de0d41208506caa461a4ae8c7d05df3416a7069fd81b0` |
| `data.cpk::skill/0C5.pac` | 1 | `762ad6207237d38697d0a44fb9fab6f4fdc805a6ecc55224a812b7cbb0c88c89` |
| `data.cpk::skill/0C6.pac` | 1 | `fc37963aacddb292e94dea6e22fc9c850ed3c6765ab1e0ac58db896b8e814f96` |
| `data.cpk::skill/0C7.pac` | 1 | `d75628b96cf4d00be5fadf26c80c5f550334a63dd70efe104f967f68a42648f6` |
| `data.cpk::skill/0C8.pac` | 1 | `0cc78cea1ddfc9d4d793db7901cfdf967a5c32c7445519e371f1182dcc0e4a90` |
| `data.cpk::skill/0C9.pac` | 1 | `35ca76587dbfae9de223862fdbe3db1052bf872d2df03a2d75c4c5ddac693a38` |
| `data.cpk::skill/0CA.pac` | 1 | `72e1f86accb1af12d6bf2d4d23e13aa7d08b9efba2e9ddd26e3d5e1a5122653e` |
| `data.cpk::skill/0CB.pac` | 1 | `2fd57983732c96e4f6bab3b9f62194f6a82904006d055019916201ccb1974a3d` |
| `data.cpk::skill/0CC.pac` | 1 | `30ad4167598c8fb071465a288570b285a15d0e86a4fc0226302796d562a02af4` |
| `data.cpk::skill/0CD.pac` | 1 | `86eaf5d883bbca39e5e0cb999599a438e7d1e9a966d8e023df0597406a1755ad` |
| `data.cpk::skill/0CE.pac` | 1 | `e07e20d5ec682d6654c86b190af26b1d44a8b80f6760a5f8515ed8875c68199b` |
| `data.cpk::skill/0CF.pac` | 1 | `ba567bde843472838c8d100808e481de0a90fdb2b492d7fc7d14fece2aaeb8ca` |
| `data.cpk::skill/0D0.pac` | 1 | `c68f0aaed47bd5e80e410910b1f9b50b8a5bf687f5959a7bf965f4a510ac241d` |
| `data.cpk::skill/0D1.pac` | 1 | `f105ae87b7da23b930aac70a8c87dae4de4b13ce078f033877a793a0843fb502` |
| `data.cpk::skill/0D2.pac` | 1 | `61efc8fc6a0251756d695d5f9e916de89bb83a5f2f609e3e01dbf124aa8d558b` |
| `data.cpk::skill/0D3.pac` | 1 | `aead13864f1d709ba9ed5e6b3d3c895812a87b39a6049cfac44c101000a0fcd6` |
| `data.cpk::skill/0D4.pac` | 1 | `9bce7cf3bd708977e21417713ba031eb75e628cd7ed8f51d6ff2ccbc862bc727` |
| `data.cpk::skill/0D5.pac` | 1 | `6757462fc78cb3c0e36acac240592907fb8dd848b8774a2d7798f01a7791bf4d` |
| `data.cpk::skill/0D6.pac` | 1 | `664f9359d416e86b251b8a6cc467523c50c96e7bc453d42e4225ebc34a5b7d45` |
| `data.cpk::skill/0D7.pac` | 1 | `658e2ea59c7e3a1d3b44627e9ebbbd4eb7d4226fb5c59b7ad1c1339155ba5b8c` |
| `data.cpk::skill/0D8.pac` | 1 | `a725d496774e207af33a729de7977bd85afd6edea23daf08d4ee02b3731286c4` |
| `data.cpk::skill/0D9.pac` | 1 | `98cc361f6c19345218d47c4e64c4be6cc7e01550e5e60ce1d9600b02384c4005` |
| `data.cpk::skill/0DA.pac` | 1 | `4bace28c198087e18322a70434b8c419909289d4f3dad1fb39fc952866fbdf55` |
| `data.cpk::skill/0DB.pac` | 1 | `49c1dfd0c8e2caabb3c56874962648eff401252da7d8521335631a495cd660b0` |
| `data.cpk::skill/0DC.pac` | 1 | `281294fcde2f460e3f254384cc4722a389ae88e6b807b55d141da3fbe595f05e` |
| `data.cpk::skill/0DD.pac` | 1 | `28d53abc010848e2a6866afbded0ca649460b95fd3e051015b3dec81adc3936b` |
| `data.cpk::skill/0DE.pac` | 1 | `e74eb55bbf716fc1c1883ddec77549688f30a4c8ab0a7b9be0942c6fab1ec604` |
| `data.cpk::skill/0DF.pac` | 1 | `10fe6188c9eb25820e98b83c79ed3b8f0cdddca4eff49474339a672a19f77931` |
| `data.cpk::skill/0E0.pac` | 1 | `5e0c4fbddd1edbaedb38138ae6e631dce2f573c09e1a08d203cca2d5e71306b3` |
| `data.cpk::skill/0E1.pac` | 1 | `1e5448c497ef5691265416430516a8974124efa102a6763fb7b7151541746fff` |
| `data.cpk::skill/0E2.pac` | 1 | `89285f367c4b70efbbb1c7bda92e409bb4fc9f7d4ed5a230e7857dac287594de` |
| `data.cpk::skill/0E3.pac` | 1 | `35db79734012d5751d7ca4c0fe7752ca494fb7dbdbf378952fc7caf166e0dc84` |
| `data.cpk::skill/0E4.pac` | 1 | `6b253aedc55694f51d005dc47afe69ab8c3171361bcc64f15a7eebe96506da28` |
| `data.cpk::skill/0E5.pac` | 1 | `a2a00c0476efb61a0983003368103fccac155f226dcf86185a19de364403a430` |
| `data.cpk::skill/0E6.pac` | 1 | `57e7216fe25aa42b3d47448bca6ba68435391ca9f50632f4dd3cbe6b5e8ba967` |
| `data.cpk::skill/0E7.pac` | 1 | `87c7103ba8f3a1fafae6daa49460271edf739c2b008973abea407fc4c95c93df` |
| `data.cpk::skill/0E8.pac` | 1 | `c12a4008275a64f6910a383d8e564f5713ebe967020120c856c2cf4bf8529925` |
| `data.cpk::skill/0E9.pac` | 1 | `034870e1d03f96b9f9342acd0e818e31d14d6c3feff7571a917cc071a6c79a62` |
| `data.cpk::skill/0EA.pac` | 1 | `6ce4c02ececbe030f9f3bc3be1dbea9eed48f82e79592142202575d40e9efbd1` |
| `data.cpk::skill/0EB.pac` | 1 | `df76c5795782edf0e77e434891f35439f61792640fafa88f5a1be974a841e095` |
| `data.cpk::skill/0EC.pac` | 1 | `1f50162fb67805f898b546b421f1542e390a450b6fbb87238e526d428be38f22` |
| `data.cpk::skill/0ED.pac` | 1 | `66bbdce15748b89977e1ca2f89fa67276f0c3b81809e7779bb348469d6be6673` |
| `data.cpk::skill/0EE.pac` | 1 | `8f6c6394ae2dce4c9df6ea17449aa83e8754ad72d632050420f566a4f2ee8b43` |
| `data.cpk::skill/0EF.pac` | 1 | `3b53cada536fa986bcc5aba37ecdb34aa8e6839f8697ae6f61f8c8656691ce17` |
| `data.cpk::skill/0F0.pac` | 1 | `3e2614fc6061d9fcd8f41bbaa34b139a7af8981ce3c198a0931370d490c969f8` |
| `data.cpk::skill/0F1.pac` | 1 | `6a92d1c88a9b1ba6ad553031079350a2579293d36a260684b60ff28af64a0205` |
| `data.cpk::skill/0F2.pac` | 1 | `7c692382c0c33186fe1c454dd931645d3f6d82d88d07211cd6c4b0acc455dc66` |
| `data.cpk::skill/0F3.pac` | 1 | `c1438dd1bfba08ee1f26257cada4d073477703f0ea5e938f7882d0b41cceae21` |
| `data.cpk::skill/0F4.pac` | 1 | `d86a8d3ccd2ad3d15f47f80362cd5efe991e52761355866afa401464621c2547` |
| `data.cpk::skill/0F5.pac` | 1 | `8ad2893fa5f5ac1e8bfda34dae0290bdf466b0733a0a064319658f3eece477b6` |
| `data.cpk::skill/0F6.pac` | 1 | `929d4881d5eca769fef7db04af1a91bfd913830ebb1e89a35197790cbcef1d7f` |
| `data.cpk::skill/0F7.pac` | 1 | `072d0643c994bc6763a4a261fa9013c029e71fde5982d06f8aba4004c012a05e` |
| `data.cpk::skill/0F8.pac` | 1 | `7edd0d6ddaf40e0d2ca67fdbb5f623f3d4ea482c68318529001eae502e3780a7` |
| `data.cpk::skill/0F9.pac` | 1 | `3916ca58bf3e993d442e36abc434bc99c69cf322e5a1eef2f2433b2e8cc2db64` |
| `data.cpk::skill/0FA.pac` | 1 | `9539969b159ce97a9c6990a1b2d97e41d3f0881abf122656a4d4b52d1655db12` |
| `data.cpk::skill/0FB.pac` | 1 | `4f5aa6be16d055c5c1c8175ada36243f7e8e3b47c6b80ba186832a4964e57768` |
| `data.cpk::skill/0FC.pac` | 1 | `247d56c4132f1919d06f0ef4b4a0ea6ba9d553132505c6af897ae94036c4e007` |
| `data.cpk::skill/0FD.pac` | 1 | `579a3ead9ff5683f3d0b4ff23d712ab556010ff43112f30c5b00a9e9f6e6fc20` |
| `data.cpk::skill/0FE.pac` | 1 | `7d205bb8cec312569b94a1f54c8dc7509cc2e7ed171b0d073d8bb02b75109e71` |
| `data.cpk::skill/0FF.pac` | 1 | `5ce781f8704a2785b8fa0078eac7222f146ae7ee29aa03be2022aff677d19a8d` |
| `data.cpk::skill/100.pac` | 1 | `523369ab152e65252161fec93cc1646b6b478fd4d68fb5a02c644c013709926b` |
| `data.cpk::skill/101.pac` | 1 | `205b75881dfe68c4d64e8eeb4bf092a39c5e17153fe8a3dced8f194e760e0983` |
| `data.cpk::skill/102.pac` | 1 | `ba8754af7b57b822b18f4be44f8313cc1d7577129541407fa69bf043c3452a12` |
| `data.cpk::skill/103.pac` | 1 | `c8e93d624949c3cfefdef2a55658c72f13a16f159ba735f5b86d42cb1c2c3909` |
| `data.cpk::skill/104.pac` | 1 | `e0b233ba4a014ef5cae6064b58203494ea86b676f61cfa010889f1b494cc9887` |
| `data.cpk::skill/105.pac` | 1 | `181265b508f05754962346a07afaa8e138cb49eee6195189694a747a515984c0` |
| `data.cpk::skill/106.pac` | 1 | `2f1d3ca2fc025acf62ef946e2453335de47aae290743f494d64d0b9ce17f7261` |
| `data.cpk::skill/107.pac` | 1 | `e073ad473717183793678b49b981cfc962f4af4e24423b013c2189b02c53f27f` |
| `data.cpk::skill/108.pac` | 1 | `9794c2c359ca8f13187334edf62068c77b9629601d2d0fe162c0e4dbfa566436` |
| `data.cpk::skill/109.pac` | 1 | `754bfccf2eb1cd6289f6598913cbc9128e3920a6523096708fcb0c4356d67f43` |
| `data.cpk::skill/10A.pac` | 1 | `35cf62a94405978712e9e31bd66a2f0dfd19ee7cc8455e7f1bdf10e403e4a4da` |
| `data.cpk::skill/10B.pac` | 1 | `d91e81c067d459c62fa41188c05811632ccb902935eaf66719dc43f47e7fba10` |
| `data.cpk::skill/10C.pac` | 1 | `7a911332d1a4aa1f213ad87fdc70faf56665f4ef9499e9e4dca122ff7ae02421` |
| `data.cpk::skill/10D.pac` | 1 | `dce0b4cf445489452ab69a0b860e3d65f74d6da57a2f7e8583cc61c6a8a1685a` |
| `data.cpk::skill/10E.pac` | 1 | `fb48f3ac2166325c917704bfd842ae63d65ad5e887ae4cc72c57d1a8c78ba33c` |
| `data.cpk::skill/10F.pac` | 1 | `0d2ba5ace12b81cfdc21702ab329ba963f3547708dafc5ff34f46892b3e95e3b` |
| `data.cpk::skill/110.pac` | 1 | `7963ba6af172368ab4f9424d6e2e6b73650220a5a7b43982b8c94730bda158e4` |
| `data.cpk::skill/111.pac` | 1 | `90a01bd09e0829a881cd8b94ca8c57d59353f479f749f6c21e5509dee680e01a` |
| `data.cpk::skill/112.pac` | 1 | `d58ffb312989480d06250e5a88804daa4e59d5711cf98472c9d62619fe9fff08` |
| `data.cpk::skill/113.pac` | 1 | `45e7601ac0e923de6dbbacb9d001ddfb29927f2157e63d65fce25f31e0863ab5` |
| `data.cpk::skill/114.pac` | 1 | `50aaf306bde98cb34e7cf5e745e546d8035cbfcc243fc3c33744bc91c5ae17b4` |
| `data.cpk::skill/115.pac` | 1 | `69a4f29bc51fb3f2558b28acca095c301c2c8a36cf26b7026ef146df35665fea` |
| `data.cpk::skill/116.pac` | 1 | `3a17d9729fac7cbbe44f5961c5252ae095ed1ca0999117cf40ebe71d35b2d4d7` |
| `data.cpk::skill/117.pac` | 1 | `6f2bb6feae8f5d7b713ff83d0349b7a0d91ea07784b094013954942394ac2b7d` |
| `data.cpk::skill/118.pac` | 1 | `32456ee8dc3e6a842000ef3052ca3ca60ade8522a56fdbaa71f3d4958d7beba4` |
| `data.cpk::skill/119.pac` | 1 | `035b090eff476a53112c231e00b1cb56619facddd88906ee0d24b6ff3616504c` |
| `data.cpk::skill/11A.pac` | 1 | `ffee7c074693fead1b22ee79ad3ac19ca57c333f4cd555f7165cd5db9bd3dfaf` |
| `data.cpk::skill/11B.pac` | 1 | `f4192ff274cffcbc5753caad2be80fb95da2ebda3b98ddc03806defac6d37e22` |
| `data.cpk::skill/11C.pac` | 1 | `ce09686f76cdb6ec2dd38c0de8598e9ebe5252902363ba002540070d95ad6ba2` |
| `data.cpk::skill/11D.pac` | 1 | `19862ea83452eaa3ab8b4eb1290f11abac92c9db613fb8f68f4a438d3edd2021` |
| `data.cpk::skill/11E.pac` | 1 | `8cf4a2ce3dbcb1a5683118337043882ff2d3a3bbedaf80368adfc21ed3d82042` |
| `data.cpk::skill/11F.pac` | 1 | `6d3a4bd51e7af1d0a253f223e30dc4e52fc79d28489d2ff0e79378f60de888e1` |
| `data.cpk::skill/120.pac` | 1 | `23e77f9ce08c5e14b9d9bcaa3a534d109f4a9f00e1728301231e5ce445ef8e51` |
| `data.cpk::skill/121.pac` | 1 | `62a074ebd7b970a4721a979567a92b55be42e3ff06756c1bd01f7778005c6334` |
| `data.cpk::skill/122.pac` | 1 | `d0d371a85d25ce172a77d74476332ba041ed7c733f33cb2a60d97a78e8aaa702` |
| `data.cpk::skill/123.pac` | 1 | `a8cd906905637aae0d15bd265e6419e9e0770ce3de9508ed99ce9e66ec294a5d` |
| `data.cpk::skill/124.pac` | 1 | `4f369a2d218f1fc90994c511138c5eb1b77f2521b5b455f5d3453c9101a64885` |
| `data.cpk::skill/125.pac` | 1 | `761b39de81d0db5ac82a6b455efdec0eac00c29db1fd9cc1e78819310a865506` |
| `data.cpk::skill/126.pac` | 1 | `95b3c5bd8a61e04c85570b98cd2372b1688c770ba962e3f884b6c2aac022db86` |
| `data.cpk::skill/127.pac` | 1 | `0a138c160c2ed1d26a8a700e6503da88b604b8ceaf8f2f4469c2d10ae5272225` |
| `data.cpk::skill/128.pac` | 1 | `6f9e7b783df2973c6ea74ac99eb7ae94d764815c32a9b812ebe456474ce19a18` |
| `data.cpk::skill/129.pac` | 1 | `5c1b741995206f623d662c186b37d2ad9eb2ccd21359eb22f7ef02dd7ef8ebd7` |
| `data.cpk::skill/12A.pac` | 1 | `31fc4eb2947272db6bf14a5299fdd5046188b5af6f06f8fbc0a3b0cdf951a41f` |
| `data.cpk::skill/12B.pac` | 1 | `19de9918e9682dbd86e25d6ec390f6b57b434d569c09721d03c4f37fb708c228` |
| `data.cpk::skill/12C.pac` | 1 | `8f8d669c8c2dd3d20e551efcef796ad7aae677467a818d3deaf295cdbd47b5c1` |
| `data.cpk::skill/12D.pac` | 1 | `e47dca51c585354e65353c51011139b7c820bf9894d1aeaa29129ab03cc93de1` |
| `data.cpk::skill/12E.pac` | 1 | `ebd22d2c75900d186f74609b12666eb8584c7a69c6b03e1d2b0c2d8b5531375f` |
| `data.cpk::skill/12F.pac` | 1 | `b997f1c3f8a86231804cdd7155e6e9875aace82362e821c01688f833e31d46b1` |
| `data.cpk::skill/130.pac` | 1 | `a1e0541b68acf8fc939cdcfd8864caa71d11fa2be538254f96eeedff4b2b533b` |
| `data.cpk::skill/131.pac` | 1 | `aa1577bf1f7dabe3206a5fa40740eb92bb83abd2d10d4bdd8d7f86e0e9f48953` |
| `data.cpk::skill/132.pac` | 1 | `d94efff2911cd6c177a92d146b5bb3e329c8bfc3f0a0684c701121c9db179e06` |
| `data.cpk::skill/133.pac` | 1 | `e29386af60ab09b10b4a9b6efac183589d8c7cb2e1bcaeb1597de33555222bbd` |
| `data.cpk::skill/134.pac` | 1 | `6fb3afb1ac4a36031b8ca944372765f687009ee96f2441c9700047bd26061e01` |
| `data.cpk::skill/135.pac` | 1 | `07099d666028e5e47dd9a243bf8d091a3081aa845a787c272649edd2a99367ca` |
| `data.cpk::skill/136.pac` | 1 | `cf13be62d221f01c16f9d954948fd251f46b3a519ad4e009c97ec5a52509fc43` |
| `data.cpk::skill/137.pac` | 1 | `6636680be91ba0752536845b7639247d7c1633d04b0c8715132af5457d21d5e8` |
| `data.cpk::skill/138.pac` | 1 | `b99e1896f90c78241b8bc788932d10c299a4646b380c1e31bd008a0881efa380` |
| `data.cpk::skill/139.pac` | 1 | `77002d5c686fc9d90824570199712937778c18db163de2ccf80818fc12523c98` |
| `data.cpk::skill/13A.pac` | 1 | `dfb2347b42235ac06ee821e029b95b2d6677025c548e5212ee4a901862cc4179` |
| `data.cpk::skill/13B.pac` | 1 | `bea337e7029e29855ebfd7342a7177b056bb3b2b3f0768d341f9487166a3592b` |
| `data.cpk::skill/13C.pac` | 1 | `4765b0879f7815714433de518cb4bd573e29ecee7d050a12f50bc509dbf28dc4` |
| `data.cpk::skill/13D.pac` | 1 | `988b505b3e875b2cf0c96cfb0cb85e132fe22f535f47716dec46b8b6b55b93fd` |
| `data.cpk::skill/13E.pac` | 1 | `6a0e79963034d74cbc457fbe22cdd922f2b630a2e6ffe821f3b55551a6b18b7e` |
| `data.cpk::skill/13F.pac` | 1 | `bbab72ad8f12b32fc74423bd7f56308e75334432073a893ecb2e8890f1c58a4f` |
| `data.cpk::skill/140.pac` | 1 | `ec31324746f3d5421dca71132fad60d1f85b5dde4e1c1ee15c0084e88d6a07a5` |
| `data.cpk::skill/141.pac` | 1 | `ea487eb60f64fbb0762664718e80c8b155add396a8c9241906dfe683498d684a` |
| `data.cpk::skill/142.pac` | 1 | `6bb40f0bc695da17445c38dbfeff4716624945cd2a70a389f4c89e5f8ffa9819` |
| `data.cpk::skill/143.pac` | 1 | `2c208ce1bcf9c66a75e1134b2915664d9531a0d6f9ce00badefc81a20d48d028` |
| `data.cpk::skill/144.pac` | 1 | `c37aff00af9e900740078ba0d1f800301493d4b11fe85daac1bbda9bbc069f10` |
| `data.cpk::skill/145.pac` | 1 | `1e7ba61d8641f28e57b0fe872150fe6e50de331b95161e70215f4328e8dd5c6d` |
| `data.cpk::skill/146.pac` | 1 | `2b0079e99952b555d127b3284ca342224fef5ea85ec66139580564c92995f75a` |
| `data.cpk::skill/147.pac` | 1 | `49b76dc2b9ed9895ae70ddb68aeeb51970a9cd8b88762a355e9c3d60544a13dd` |
| `data.cpk::skill/148.pac` | 1 | `13cbb8b6f1919a3ce691afb577f3b3c1dad32b4c9b6d47aff6bcabc7afb75f8b` |
| `data.cpk::skill/149.pac` | 1 | `3e1271438e653ffb439897e9bd2c8821d950f03e132efac7e7f3ca874aa29e0a` |
| `data.cpk::skill/14A.pac` | 1 | `46f99c5c8a44743e4752b349f6b66413fd72caac4cb66b8328e9f56c0d4f6835` |
| `data.cpk::skill/14B.pac` | 1 | `25ce61177bbf1ca9e91a28c2bc8f14e73dc10e9e4521955488ad1fc870f2880d` |
| `data.cpk::skill/14C.pac` | 1 | `368fe3b3bd75a08242b8746823cc9f9178b6f4100ad75e650ddac7b266d45243` |
| `data.cpk::skill/14D.pac` | 1 | `a366b7cb2f91092ca36e7cc8fb3b6f70a6d9842aca32d902ab613b8c7f93f775` |
| `data.cpk::skill/14E.pac` | 1 | `d5ce7b329f5d8270bf732b629a839323e68f9eb2742bcd6e0fe55cc9992db206` |
| `data.cpk::skill/14F.pac` | 1 | `82926ba3337b18659fba71247089f367ddb2219cbf71d675518984ad31df5d6b` |
| `data.cpk::skill/150.pac` | 1 | `4858a40753970bae49cfc564925c88938d09193c34d203dd942a2cc205cdf925` |
| `data.cpk::skill/151.pac` | 1 | `d4be8c788f5168203a215d518f41de3afe9235db522c77b830bd5b8bb79747f2` |
| `data.cpk::skill/152.pac` | 1 | `e32707b9a71ec51f1a8dae68be3abe4ec9388e332db63290daf74ff0c8bdc28f` |
| `data.cpk::skill/153.pac` | 1 | `70fcc0c492375188cd057ab2eabaff9ca5768e6d0713ee9d81529007d1d57113` |
| `data.cpk::skill/154.pac` | 1 | `5f307439e9bb4192c2263716a627dd0eb208b0337fa63000e47e0eb465729c7e` |
| `data.cpk::skill/155.pac` | 1 | `6da27c456db003c10409a590cdbdaedeb7ad520dbefe187bfa0452560fde94fd` |
| `data.cpk::skill/156.pac` | 1 | `9b167ed6ef60d8197b71d28be0dd308b13cecdbfc7d9b46beb83fa652c83c89f` |
| `data.cpk::skill/157.pac` | 1 | `ea6518ff400b7f25bf22bd574ae2ef106c85d83c5a9fbacae0271370b53e6775` |
| `data.cpk::skill/158.pac` | 1 | `f84c43b1069a0e7245e340098a802e3dc2f600e288282b7461e51031cf7fa430` |
| `data.cpk::skill/159.pac` | 1 | `eccc113cbec0d4ac156a65d88f5097212d1379c222e6f83b85e39427249d4aad` |
| `data.cpk::skill/15A.pac` | 1 | `47f4130936ad06a69abf05a46d1aab6983b2f2216106be4eb9df64496936eca2` |
| `data.cpk::skill/15B.pac` | 1 | `c2a2fb464a0eafb3b8329eb5dd442c68a310a347d49e7f519f648c5e16d74f91` |
| `data.cpk::skill/15C.pac` | 1 | `036523233f61b89f710bcc44e182736eaeac545858a9a5f71d68f4a5d8a16ed3` |
| `data.cpk::skill/15D.pac` | 1 | `acee6a82bf4e9d8df948a9ac0b82ba22a155e77245676fbf9d71d4ab41eb6ff2` |
| `data.cpk::skill/15E.pac` | 1 | `820e5402df3bcca5be7aa3cbb513eb6ea6d0faedf35c3ab9d592cc10480cf885` |
| `data.cpk::skill/15F.pac` | 1 | `48f16d9b709d2289eda46a2808249f2f2aef5f69f6b2af7f364017fa4c5ce27a` |
| `data.cpk::skill/160.pac` | 1 | `467b12c611cf74f54110bcf1aa8300d74973bc252b8a1e127f8a0bc19eac538a` |
| `data.cpk::skill/161.pac` | 1 | `db86af237a4a5c0889e2c4703f64414270db40622c049e0bb9e6c78b2f0600c7` |
| `data.cpk::skill/162.pac` | 1 | `c7090c5ccc0394294dcd459927982911572108a4b19cfc4dd60ee0bd6a59cf96` |
| `data.cpk::skill/163.pac` | 1 | `7227e6454fd7d4bee2d1c122736f70d76c2820745714d1386f94e7bfb18b448b` |
| `data.cpk::skill/164.pac` | 1 | `2ad637f51113bbd022d40b62830de31ee5ec5162265ad747262e295104abd914` |
| `data.cpk::skill/165.pac` | 1 | `34716924d132d61a8dc36e7a2ddedf9077f02773cf8032fd357f2f5fdcebe2a0` |
| `data.cpk::skill/166.pac` | 1 | `cd7c083c23a2b9d9aa38bd0ee8de8156c1fe966d0279ae7768a0f0c054a41763` |
| `data.cpk::skill/167.pac` | 1 | `2c712bd4fbce9f4218aee13a6ce61fe2fe0bb61f20d94eb1cb94bafd44ea9f56` |
| `data.cpk::skill/168.pac` | 1 | `19b26663c920d523b5929b8bc9fb8cf2defb080fdd3e3dbd038a23675a7a831d` |
| `data.cpk::skill/169.pac` | 1 | `41bf69cb5b303ef17a358375159f9ebb6fc34520c1ea3eda57a263b9505cb690` |
| `data.cpk::skill/16A.pac` | 1 | `0a1f1861947c565b06732f664d02dd95eac21cc68797be7ef3d80d8a62705381` |
| `data.cpk::skill/16B.pac` | 1 | `a6e9995da2dbfc02cc441306e14c81b0c487662916ab4d34ce48c97080c67f0f` |
| `data.cpk::skill/16C.pac` | 1 | `d56f865b018e06315ead0c1e828526e0255347cb70ee108113e3ad0dc68cc14a` |
| `data.cpk::skill/16D.pac` | 1 | `41eb137edbde294aa5bf2ace7ca4a85b6863b4c79ae609d84c26143a3a144c70` |
| `data.cpk::skill/16E.pac` | 1 | `3cc21379a6763a0207335eef6f0e44a534c5d897949b4a05ea96b796072cbb45` |
| `data.cpk::skill/16F.pac` | 1 | `5002961ce73f4502024eb1bffab8a29e2b1e2338f202c7d7c70d4b918252c8dd` |
| `data.cpk::skill/170.pac` | 1 | `337b8309b4a6e994370ac00f154d6c353f3e7415094f7c64eb8df6dd9d35f41f` |
| `data.cpk::skill/171.pac` | 1 | `fce6e46e9997cae07b8102a78691aec8537e8a58b0d8fe686d103be43f92c3fa` |
| `data.cpk::skill/172.pac` | 1 | `a5f8681d12bc0c482e5c0c65612bd49b0585a929adf5489695bcec3eda80ea91` |
| `data.cpk::skill/173.pac` | 1 | `3169b2f0135122af1f0f5bedc3ccb096d6f34bda2cd6104bee3a42d52788e37e` |
| `data.cpk::skill/174.pac` | 1 | `f6020393409aaa8664a46896d6514f1c8962919c9b1cb2f5e19ad685cf8c6375` |
| `data.cpk::skill/175.pac` | 1 | `3d99642780823b7cfe5ff8546f30d45659a6e9fe7077c561ae047c6b7e9e760b` |
| `data.cpk::skill/176.pac` | 1 | `afa817e516ccb2da6895fffe7bc30b8820984037e1fd7e6cfe383ba4c97c4a12` |
| `data.cpk::skill/177.pac` | 1 | `4e77bdbd8f405e1973eb279dad6df280ac9266e72a4e048a75ff70a812c96d4d` |
| `data.cpk::skill/178.pac` | 1 | `33295deada02128293cb50145980c6730e4dc7dbe2962a95c3f66a7e25b308ab` |
| `data.cpk::skill/179.pac` | 1 | `1c89a729f5ac5362fae84c452e7463cc085cc236b464194f7e1f02d179c6dea1` |
| `data.cpk::skill/17A.pac` | 1 | `44b83d2baf87dc0bbe5b4784871df2397d39f27476461dedc87e395064b1e3a3` |
| `data.cpk::skill/17B.pac` | 1 | `753b50952d260b8ca977381cee892ddf3a8bf39ccce9550f66782aec48810857` |
| `data.cpk::skill/17C.pac` | 1 | `e4636e559d9e0fc5a9151caa37526060fff7f95ed26560c7f287bca931fa61c5` |
| `data.cpk::skill/17D.pac` | 1 | `ebf1591cd04851f1cb280d78f2eee6e7467e3a5683f49248d4b480ed51ddc46f` |
| `data.cpk::skill/17E.pac` | 1 | `f057731b667f7293dbe7c737bb4d20ef6961f9075b69ebb52d0f7f0df3e85304` |
| `data.cpk::skill/17F.pac` | 1 | `b7059c32420ac86344ca8f2f47feaf30f03760aa28f946d6a5a789e2605a3d5f` |
| `data.cpk::skill/180.pac` | 1 | `a3f3c08ee6457e5c2deee1d477c29b12c0262f257a0631f9dfa639eb5ccb41c2` |
| `data.cpk::skill/181.pac` | 1 | `ec0acf7ab9c16009064cc63e518537139d07b7adbdd50626af3f9c96ea3c11d5` |
| `data.cpk::skill/182.pac` | 1 | `bd939cb77f54bb00a8f6866f26e656d52c952d2f69978832ee07aa6e0f75aa3c` |
| `data.cpk::skill/183.pac` | 1 | `506c8fc7e34cec0049cd7783f24e4f993a5de7bb7a6e539b0f0065d627016eac` |
| `data.cpk::skill/184.pac` | 1 | `8406327fb0ffd2ddbacb9979d8e1eef646ee2c0549eb302a05b486b415a25226` |
| `data.cpk::skill/185.pac` | 1 | `0b7faa42a0c7f3f94e10faa1b62e26c51c1d85f1aa2b97e2229d4b76fe6c9932` |
| `data.cpk::skill/186.pac` | 1 | `b8e6e328baeddf615b1bb41f7cf2028e8fcfad06b65c5d6204e8ba0ea739fbb6` |
| `data.cpk::skill/187.pac` | 1 | `e72ca9e1453c834fb56ee734f21d27e1f47a58174b274b7e9438235747baaef2` |
| `data.cpk::skill/188.pac` | 1 | `d24c2eb57f6bcb108ce4cb4e54b95391d2e4b4209b898422dd0739c686246c7f` |
| `data.cpk::skill/189.pac` | 1 | `2343ce8a85b7d3fff0c488fc00abab7fddc4af120ac86b68712239b4c9717913` |
| `data.cpk::skill/18A.pac` | 1 | `103e92d44beed3947deb4f481015b2dc6ed3523b26c4fa47e688a743ec4dc437` |
| `data.cpk::skill/18B.pac` | 1 | `b50d9606df14df257917c2d0aca5c8a270fd412336ea32bb899f5add4e19eeaf` |
| `data.cpk::skill/18C.pac` | 1 | `3bd80885cdb0447ceb1266bcbb2ae307ec6ff69c2304126c8e3613c955441882` |
| `data.cpk::skill/18D.pac` | 1 | `87dbd81d3939249e62f2d630ff953accf81c55a2bafc7d6f155cfc0b5fdc6bf1` |
| `data.cpk::skill/18E.pac` | 1 | `6e1a9fde49b5ab2f779acad031480ef1b7086478034220ad7e8bf5775616af53` |
| `data.cpk::skill/18F.pac` | 1 | `30a7474992f73708937dcfec7ca89461e75de5dbaca847a66ae613f794886305` |
| `data.cpk::skill/190.pac` | 1 | `5850a5f79a3a389f6f5f2f002ef5c155129aa6c5b363bbef12040ec3d39d6565` |
| `data.cpk::skill/191.pac` | 1 | `d700abd5a295dbfc0ba870920210f82589a4f1cee80bbd291c0a5e304e3b55d6` |
| `data.cpk::skill/192.pac` | 1 | `2c318b2a4c5f7f2cc16d537fdad5fdd025e0a9755c5d01deb617cc51627911ab` |
| `data.cpk::skill/193.pac` | 1 | `0e090a6f3361627df6848ba28cd0df0c744e800a852216ce574ac6b8d89e4da1` |
| `data.cpk::skill/194.pac` | 1 | `5d3ec1e96d49b13cd9d91f00519591fc702f44e380cc300f622b0263bcef59e2` |
| `data.cpk::skill/195.pac` | 1 | `00490e00b0d5341eee10219c38cc1e061234348f18ef7fbab3518d7cc9ba1188` |
| `data.cpk::skill/196.pac` | 1 | `3c82a03cb49e23955a88419ba4bea826758ced07ebff434841d10a19bc449980` |
| `data.cpk::skill/197.pac` | 1 | `9455e0746bcaff326550495862cdeb317302d6de829e5625fcf6033be2bda70f` |
| `data.cpk::skill/198.pac` | 1 | `60a9c9a7f1baf0f1a3480c97aab517ab35a57f0f18172107f7fbbb24d45b9955` |
| `data.cpk::skill/199.pac` | 1 | `a9387ed6e65448b86bb584049c98960ee2944386b020575b9f0979193c5d043a` |
| `data.cpk::skill/19A.pac` | 1 | `a3f9e4dbc73b3bdf3ce5fd287e8f4a90e2cfb78fc5945b2ade59a54d38a72f43` |
| `data.cpk::skill/19B.pac` | 1 | `eac7b4926b65d1e9ffc8a1de8af450736eb6c8f3f1b97e2e8a5f495c6c5284ac` |
| `data.cpk::skill/19C.pac` | 1 | `3b5a1370bf33661c6bd991bc084bbe103d4314b039bddf960a2b7c75a6a7b6f9` |
| `data.cpk::skill/19D.pac` | 1 | `1cead0ae7dc2e24bf29253a2397f4a79a0488e56b6bb8ef3ec3066b962f4ccef` |
| `data.cpk::skill/19E.pac` | 1 | `3ac9735c867f64fa2d2a50a176765a6eba8fd0ed1e7aa3bbca717fe60d72b489` |
| `data.cpk::skill/19F.pac` | 1 | `328db4a978c4d97f1b14c89bdcf0db67c888b9829667457f9ac1b0b8c6bc335f` |
| `data.cpk::skill/1A0.pac` | 1 | `413a7e36dc7966782ba6a381ae3ccffcacf2f20b7b7cdb25e0ba71bc9eac3c2d` |
| `data.cpk::skill/1A1.pac` | 1 | `8890fff211984db6d52f8bb43b025bbb848e9d6132d2709c17476bfc4baf7fcb` |
| `data.cpk::skill/1A2.pac` | 1 | `1dee19ec9cc9ea6448d31c4826ec254236c825c1511d1b45b777742b736bfd97` |
| `data.cpk::skill/1A3.pac` | 1 | `448444523831678ab099cfed6dad9ff914cdcb1e3d6864219593d829e833921c` |
| `data.cpk::skill/1A4.pac` | 1 | `950fbf55415fc409925432b53fbd2cf5440287fe22b18c499df9c2864a63b471` |
| `data.cpk::skill/1A5.pac` | 1 | `039cc535b6b0a02c46c2f9601db698c204d3ef9454e9fb4ad9423f7f0de9c2f1` |
| `data.cpk::skill/1A6.pac` | 1 | `1f469362c3fa4097073013389ce9c4f9cd1c07ccfc549d2c06d91b972b5d2de8` |
| `data.cpk::skill/1A7.pac` | 1 | `3cd2c5f41eec592e02d6bafb9e0d2b38d86a02c80105d6e134c6fb1463035671` |
| `data.cpk::skill/1A8.pac` | 1 | `46cee0ede6f8c35107d73265fc7b44ab1958b70d0985a92a6c18d97994a75f34` |
| `data.cpk::skill/1A9.pac` | 1 | `98d433a0ae0ea86cdb3c82fa0abffdb8097ed82c9afdf3fa4cedb15ca67996f2` |
| `data.cpk::skill/1AA.pac` | 1 | `284c53db2747c2d96a7bb74a9e63702ea97a61d439db82118d66dd1558b4820e` |
| `data.cpk::skill/1AB.pac` | 1 | `f3f9220c0624feb2e68c9b1ec6a912a2eea13c0a0a5314401bdcbd502e929874` |
| `data.cpk::skill/1AC.pac` | 1 | `03bf0a27b12c9274c76fdc1ae3f972f887418aec8e35d36cc6a697cd9d3a88c4` |
| `data.cpk::skill/1AD.pac` | 1 | `6ae886e7a9707d9f3f986f6c5c7bcd038e190203fdba85d8ceb552a4a901d116` |
| `data.cpk::skill/1AE.pac` | 1 | `c1bc23ce6e68876678dc545104377b28a4f6d27558f91adb3a3a476a3f235d12` |
| `data.cpk::skill/1AF.pac` | 1 | `f3f0236784b3cd5de0bf43bcf3a469032b9b122bae83f063de09ab9767914eb5` |
| `data.cpk::skill/1B0.pac` | 1 | `39a2e60694c6465f8e352ae9dbe2fa93f959704f9644c565ffcdfc8b7ce1c3e3` |
| `data.cpk::skill/1B1.pac` | 1 | `4089e575318a0032b07e7b53e9f693a1cc53ad3ef4e3a798f951f7d43a753c96` |
| `data.cpk::skill/1B2.pac` | 1 | `2d019d5f62755a895c5d6528720865c5dcdde87a9d23d8641413e6f2f054cfe0` |
| `data.cpk::skill/1B3.pac` | 1 | `b5c85a95d0c8eedadb894c53951ec45b75231ce1295a4ffb01de9e2963dc1d8e` |
| `data.cpk::skill/1B4.pac` | 1 | `f37ca44997b76febc99ee7741290e049f285c5c30d8eb2200c34229156d86156` |
| `data.cpk::skill/1B5.pac` | 1 | `0286bb0dc7b53a51c729ec6b9f011d865fb16b63b257323a62aacc67eaf3b32c` |
| `data.cpk::skill/1B6.pac` | 1 | `641f1238132fbdde8b257763e17da8470298f055a8678090bc205ed4b31c1881` |
| `data.cpk::skill/1B7.pac` | 1 | `1667dcd266a5ef51fafa1535fe8776c97298f1005f015775fad9396c7f351998` |
| `data.cpk::skill/1B8.pac` | 1 | `f7eb7b8907d84f92bcbad714ab299de95bbfec30205158810ba3ad47819835d1` |
| `data.cpk::skill/1B9.pac` | 1 | `645fca64f499a94c8935d78f726a1890c0f0cf6d9edc53f1c65965abce75d99a` |
| `data.cpk::skill/1BA.pac` | 1 | `2c1d078e28308cba18e452c80f7827912db4db0e4ff2d9f4ad88d7b4cd96fa0e` |
| `data.cpk::skill/1BB.pac` | 1 | `fe026be694d1874141f4301435247b2918da0235954f7730572443fa704c96aa` |
| `data.cpk::skill/1BC.pac` | 1 | `316e519b4265969fdc73368788c166936ba0733fb992d1146d72e38fa52c8194` |
| `data.cpk::skill/1BD.pac` | 1 | `9a95410af67b41aaefa9a226f77f783d9500ed6ccef408a7c00b35d0c388e9f4` |
| `data.cpk::skill/1BE.pac` | 1 | `fe88674f045b6d4f1aebfef01ddd401622b1929382d105d1a8109d1d5e68ee8a` |
| `data.cpk::skill/1BF.pac` | 1 | `cba3f08fed517c3e115f67d0f4d4018880be200055523a861064bbe15b8df755` |
| `data.cpk::skill/1C0.pac` | 1 | `713225cce2d56bf39aeb699460cd703868507621a349d54e1524a9573ead053d` |
| `data.cpk::skill/1C1.pac` | 1 | `dc6626f82fe707d343dc67f96db78bbd599acffbc71c62d4ed5c5f6bb1f9125c` |
| `data.cpk::skill/1C2.pac` | 1 | `f5fe64d4845edd2bf28c8c1799ed5d9d8af0de3ca10ef0b28a1c7a59e2c0328f` |
| `data.cpk::skill/1C3.pac` | 1 | `7009320cb7cfd5650bb3d683c4ce057b7f786e6dd73f5990cd5f4c222dddc6dc` |
| `data.cpk::skill/1C4.pac` | 1 | `b67ec94a9b204580ba53d88e066ffb277a7f80b255a588ed13a9213d6b42dab3` |
| `data.cpk::skill/1C5.pac` | 1 | `4139005cf4f367a9e5f9e5167f7a3dbbd7135537994a1d58c257245a0900b3f7` |
| `data.cpk::skill/1C6.pac` | 1 | `ae71165aa9b96d7e7f7e2b43d96aa2a1c2104c26af281bfcce2189f7ab56f024` |
| `data.cpk::skill/1C7.pac` | 1 | `a65180b1a734bccca861b768983e8aec438985ec290956c2caef6197c111ff31` |
| `data.cpk::skill/1C8.pac` | 1 | `ff3bf8ecde10e0b26dec9630a16dc81c533b3159c160d4052b507871626dd4c0` |
| `data.cpk::skill/1C9.pac` | 1 | `c45fff6d277c41c1307a8ced9e27e9c62221ff9a6d64eeef4b20857f93a9f57a` |
| `data.cpk::skill/1CA.pac` | 1 | `70386e7d7a1da4f6262f0aad8869366af7e8988bbbfebb2139a3cfda67584d1c` |
| `data.cpk::skill/1CB.pac` | 1 | `e3e9a067069ea091d3a01b13b627e7b12917f82db294bb6e8100cb57e2a7a398` |
| `data.cpk::skill/1CC.pac` | 1 | `06b357c70e37f742d44defc7237b9e1762181d86b7414f02753ab2f41eacae9d` |
| `data.cpk::skill/1CD.pac` | 1 | `c85011501945a66e48d52bdf23cba89ad03a42e3089677b0ed40d499a33805b1` |
| `data.cpk::skill/1CE.pac` | 1 | `182ca4435c85e0dc3356fa90d049a5d4bdfbdcdc6914439fd8087ed60a7d1ca8` |
| `data.cpk::skill/1CF.pac` | 1 | `a60ad5e0e4a90edcc932d23f813d21e3a6c49ace173d513d5fe2e7809f6f599f` |
| `data.cpk::skill/1D0.pac` | 1 | `ee5fea7fb16b435ef5ece61a5550effaddc6c6ca6b0a42c9b87ead3f439f8947` |
| `data.cpk::skill/1D1.pac` | 1 | `1b484f3007496d7cf32d0b417600958e17f27b14590fec43963ccb3302bc6d04` |
| `data.cpk::skill/1D2.pac` | 1 | `ff16eec77b67e7ca4846478645a262b72db6983de8640dd825b8bb7cbe4c6837` |
| `data.cpk::skill/1D3.pac` | 1 | `7154aa3465e13ed9aaed77d78de4b7e764460e41beb43f11d2fb20db76ee435d` |
| `data.cpk::skill/1D4.pac` | 1 | `09ddcf07c4ad5b4d28f70381765faff6b9f5d23b69818c6b592a00b44670b9fa` |
| `data.cpk::skill/1D5.pac` | 1 | `20b0f16b2e36ff59777ff9e0a8666eec0790a79e86760fbf70c54008c139d82a` |
| `data.cpk::skill/1D6.pac` | 1 | `002dcfdca1f01056f1d15149dc46c2e79f5a9270e9d2b94560ce6a65e2403198` |
| `data.cpk::skill/1D7.pac` | 1 | `9dcd6a4f3771bdb47ae9c8b7ade6a4af397a1188f867babd3958c505d343c05e` |
| `data.cpk::skill/1D8.pac` | 1 | `5b9d43b7af6d3373158d43d74e1dc6207980f72707892ac9282f5f2e1631ac86` |
| `data.cpk::skill/1D9.pac` | 1 | `4b1b058b7b9a27756035cfb387f19d4bd4cf3330f8f9fe2488395092ffaa8870` |
| `data.cpk::skill/1DA.pac` | 1 | `13f6155877c43afb019ff417f3442b19b100f4a412cab0c9387cc0a2aed3dc01` |
| `data.cpk::skill/1DB.pac` | 1 | `6d067d79f829cc6fb0504b8728139dca09fde1e4a9d29516fdcb76badb18122e` |
| `data.cpk::skill/1DC.pac` | 1 | `a7ab99f4df255ec179a1d67ed35f49ba3bb0e50361f4499365ac49acceca0409` |
| `data.cpk::skill/1DD.pac` | 1 | `ac11a1ba28b6f4177dace31d15636109aa159ba6e8f8cfede199b3f2a58536d1` |
| `data.cpk::skill/1DE.pac` | 1 | `2f6d69f85f4123f9ae7e76a31f4104ab75772822a9f7474ee3704ab5e863ea8e` |
| `data.cpk::skill/1DF.pac` | 1 | `f7f682c302165e3e3f983dd0e4d440296abccadfa19739c46a9c5444c577fd88` |
| `data.cpk::skill/1E0.pac` | 1 | `7cc249364de15e245eb0f4ba44e4d7a15bb3acc3abe6e05a2081eb3582469860` |
| `data.cpk::skill/1E1.pac` | 1 | `8bb2258d47505142241b5b661b60f5d48d520bf133d52632a89bec3e7fd9e25c` |
| `data.cpk::skill/1E2.pac` | 1 | `defe23e6525bd1cce13fb4d0eda3d92f355f3c56ab3a754f937583def19d67f6` |
| `data.cpk::skill/1E3.pac` | 1 | `dd2fe6c26910cb7cec0555818550f1603df91d8f67543c5861faeee260f75328` |
| `data.cpk::skill/1E4.pac` | 1 | `4a7b938f388103fb497312d1da1de39dc35309e88714a0e5c10ca7fdba2bb214` |
| `data.cpk::skill/1E5.pac` | 1 | `53b142ad83db153fbfca0cb91a80024e096b8fb42077ec6c06b16dc84c52e882` |
| `data.cpk::skill/1E6.pac` | 1 | `95c38777b4e6c8f8432d315bf0c3d9fb9b555e2465d9024d61a2db8539a6b6a0` |
| `data.cpk::skill/1E7.pac` | 1 | `df3d92bb2e69063193623b00fe11e5371b9767522110209694200439961dced2` |
| `data.cpk::skill/1E8.pac` | 1 | `04a6acf09e997e6f1b7e14ba40d006d892e23ab27fb67598206de3bf1384d3b1` |
| `data.cpk::skill/1E9.pac` | 1 | `4b238d9e251d03bf25dfb6621940fd06b9a846e7893866288e5e9b5276689beb` |
| `data.cpk::skill/1EA.pac` | 1 | `241f19808736d53e87dc310f3dc7ea80456b70cc047fec27790c972c2efade30` |
| `data.cpk::skill/1EB.pac` | 1 | `12a2673380bf5de6d5cd7d62d281f4b612256db0b015211a565466af867b341e` |
| `data.cpk::skill/1EC.pac` | 1 | `80f0244781a3973887c1fd7e38392d1cae7d5cd4437ef82c2075531002b37c0f` |
| `data.cpk::skill/1ED.pac` | 1 | `a128e131094b3790b79dedf32334adae8d5b80b07d606751233cdc4bda54417c` |
| `data.cpk::skill/1EE.pac` | 1 | `26836b2a91a38b7eebe504b3fcefa0e139b10b461bcd3b9b352d9d9fd62c3f5d` |
| `data.cpk::skill/1EF.pac` | 1 | `1150b2af228d3cd844a5682d95d1e8acf4b983c6bd9b272906efaa64c4c3d4fd` |
| `data.cpk::skill/1F0.pac` | 1 | `80cb4092a519d318ccd243acf4befd23e91571e30d98707c84826d995b7c7e97` |
| `data.cpk::skill/1F1.pac` | 1 | `88bd33468a7889ff90ddddabd7dfd446a99c4af9a01f23065ef30666e603bc5a` |
| `data.cpk::skill/1F2.pac` | 1 | `72908a4bd71e153dc5c14d4187fb133d8b3140113d3c9601c02462c95cb2beaf` |
| `data.cpk::skill/1F3.pac` | 1 | `5636f7ab17d544db605aaa06de8a991ccbe3519e62e6af1b1cc9b3260de16909` |
| `data.cpk::skill/1F4.pac` | 1 | `6884ae629172b8e8b484b196a93dfc405a88c29fc774f12bb73bfadea140ddfa` |
| `data.cpk::skill/1F5.pac` | 1 | `97b2854b75a5fa2932ceca97c97c9fd2024b8627a6220d0806aa4e834afa6bc6` |
| `data.cpk::skill/1F6.pac` | 1 | `7d3ae8333e88a1406bb85a3d09fbc694c05f27a1b7f6b911ab3d186bb08ab58e` |
| `data.cpk::skill/1F7.pac` | 1 | `137fd865f7ebd5ca3138308379f32e025f4f82a29ede0a1a4c5a261744299cc1` |
| `data.cpk::skill/1F8.pac` | 1 | `8d18fd684a0a108156d53bd25dd2e0276b361a79f410e92d0422417172940b55` |
| `data.cpk::skill/1F9.pac` | 1 | `8c085794281af0136aee31da9acd3105b879fb4a8daefd9d80a6218948549aae` |
| `data.cpk::skill/1FA.pac` | 1 | `4ba4a59cb8e5dee2c325009eb7fc9a75b89ecaff82215c901f223cc6149c656f` |
| `data.cpk::skill/1FB.pac` | 1 | `4f04716f24c399ebb9922f2137bfbbcb14eb702051f98717951d589b932ece6e` |
| `data.cpk::skill/1FC.pac` | 1 | `93981efa848872c117163366d713482dbf4bd3c9aeee6216424f88b49cac8125` |
| `data.cpk::skill/1FD.pac` | 1 | `f7eb632e47eedb809dcc83165e96807341b01572ca5be02e9ef55d05e4e38738` |
| `data.cpk::skill/1FE.pac` | 1 | `f8e8fd049134e5e370a18d74edb03cfec866bc40c32f798f60584c6f1bf7f805` |
| `data_c.cpk` | 12 | `fdcad9007e6af09b869f425668b2ea70cd278a95f270db08b87dd6acfe787a5a` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin` | 49 | `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin` | 49 | `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| `data_c.cpk::battle/boss/e200.bin` | 8 | `284f067061d70962a95c7054187cc6f12797bdac784fa5c64b5380caf8b05f63` |
| `data_c.cpk::battle/boss/e201.bin` | 7 | `b2156d17e2480187afaa97ae096835e2bcdcf98d0515214565fa071de0704780` |
| `data_c.cpk::battle/boss/e202.bin` | 7 | `e2ef6c42956e6794b1cbc818766eb2c0fb6b95147e80ec46bfdaab6606b12c28` |
| `data_c.cpk::battle/boss/e203.bin` | 8 | `c9db4856856b487a5571f8293f4c36937f4c08fe6a5eb600c3edb26360d1565a` |
| `data_c.cpk::battle/boss/e204.bin` | 8 | `1208bf159e95b44b36d18e97e1ddd56e914531572e46ff03a20a96f7eddae06d` |
| `data_c.cpk::battle/boss/e205.bin` | 12 | `d07385bbcefe4026a57e2f077b3745af8fc17852a11c13e0052aceb6b9d5ec6c` |
| `data_c.cpk::battle/boss/e206.bin` | 12 | `b38574e1cd14da13eec21a1db2e8efaa995ff0eef3d8136f4b710de8278f3839` |
| `data_c.cpk::battle/boss/e207.bin` | 7 | `403466f766539c4acec30ba7de210b7ff857f1368a72c7fb8266f62f8f2ff1c7` |
| `data_c.cpk::battle/boss/e208.bin` | 12 | `c446dd4d64cbae2e5b53d9611a0575d718eee79bb37e813e260d96520543efdf` |
| `data_c.cpk::battle/boss/e209.bin` | 7 | `6c2e1d85bb035aa915d47a620063831348ee7dcf3a8a96e3edf5ec088290b4d3` |
| `data_c.cpk::battle/boss/e20A.bin` | 11 | `b5ca8341f4f3d563fa330936559868589034d8bc1b7303b2f0fc0b0f167779d7` |
| `data_c.cpk::battle/boss/e20B.bin` | 13 | `0ab1c6c1ac906d7aecfd4ec29256b3c0c9acaa00321a37a3b05549162b0c58da` |
| `data_c.cpk::battle/boss/e20C.bin` | 7 | `ae582a21728dc33a59ef47145c4c176029d1a251e84700061f2e0ea0e940d701` |
| `data_c.cpk::battle/boss/e20D.bin` | 7 | `fe86b4a5412ddbad06a443d6694f0c404c3c85f5b93009a560f8992d56ae2d59` |
| `data_c.cpk::battle/boss/e214.bin` | 11 | `711bcd66bca31648adf7adcbd0cf72d18285dfb7642130a45046dfa6c6445bd5` |
| `data_c.cpk::battle/boss/e215.bin` | 11 | `70a39ba489fc70aeaaafe5c01562dce33d48f2f1a6b2e1ff91f90cf1684c1abe` |
| `data_c.cpk::battle/boss/e216.bin` | 8 | `f11aab2268f3e3abd27f47f72fe14309c041c411edc296fef363f1f0cc6a9cb4` |
| `data_c.cpk::battle/boss/e217.bin` | 7 | `7384e4ce5b2b82ce480d078faf7e8401a2ae31d82b3cd2a2cb2fb75fdc0c3790` |
| `data_c.cpk::battle/result/shuffle.arc` | 8 | `e29de150a5b07f16d6d26b184dd6be913dc32528ca8b77b42258433bfdf59bbe` |
| `data_c.cpk::event/fishing.bin` | 12 | `f87e9c4d5f2d9c8afe7109be6d5972072e9527de3396409d6565ff937a0256a6` |
| `data_c.cpk::event_data/community.bin` | 11 | `820188a490b4c96e153b091ec23dd50dd1b6bf21bc601ca3f870501aed7dc41c` |
| `data_c.cpk::facility/cmbroot.arc` | 3 | `081a994ec99ed523b5818794aa808402cfa3ff944a3d67ef891f9445f1dbd124` |
| `data_c.cpk::field/pack/fd031_001.arc` | 2 | `88dba917df8275cd15886bc97d929162ef10d94e38fc7278cbaf59a28a885ea2` |
| `data_c.cpk::field/pack/fd031_002.arc` | 2 | `a843f2bdea0b54f34a6bcc9600fe2bbb5b0d6bdac9fa35c0e4e9b77cf5b6ebfe` |
| `data_c.cpk::field/pack/fd069_001.arc` | 2 | `8d1291d6c8edc58fb859a1ba637c375705d61a0b40e0aec0b89e32d9232db22e` |
| `data_c.cpk::field/pack/fd069_002.arc` | 2 | `72089b1231d29df832befc9ba60e9c906e95f332d0ee4ff146bd2ca98daa2246` |
| `data_c.cpk::field/pack/fd069_003.arc` | 2 | `bc3f148f24724fa1606055f9675dfc0c025d3757069cdc14738428ffbf3123db` |
| `data_c.cpk::field/pack/fd069_004.arc` | 2 | `640120e4b87438bf4c8f023ed7bf0e030a8b112afefa973920c1b877e49c907f` |
| `data_c.cpk::init.bin` | 12 | `ede1d3027dc586910311f09973d49dc18c1b3329fc05fa58cc9dac1a719ae1b1` |
| `data_c.cpk::init.bin::event/icon.bin` | 26 | `7799e841994b2688152ccb5a8622b104ded5903fd53cc48509d7841ce36a9b16` |
| `data_c.cpk::init.bin::insect/insect.bin` | 1 | `348d03c29eba568b87f1a7d4cce688a9b32ad08bc67280652111fb389ea8e883` |
| `data_c.cpk::init_free.bin` | 1 | `9993edd9765e3b1a42b81faee109e8c0bea9eb8fa51d3e57baebfd7bf8f0e916` |
| `data_c.cpk::skill/100.pac` | 1 | `de6518e7e7b87ca04d2510d5328f0567391582ec0b28cea92edddd5bea622365` |
| `data_c.cpk::skill/101.pac` | 1 | `7b8112096b54876bfc09bd75924bb966fb6e8983a3c00fd470f104694070bc78` |
| `data_c.cpk::skill/102.pac` | 1 | `82c06525665794fe5f7897bc2469828d96941e74602686ff798eea5f296b5822` |
| `data_c.cpk::skill/11C.pac` | 1 | `949a252e853ecc5c8f5be26993bd3f45b3c1ec521d43cc11438c26c9693f595b` |
| `data_c.cpk::skill/172.pac` | 1 | `b0f0288ba2a0eb19116fbdeb66bd274de5081e4ead456ab5ba04493afd53528a` |
| `data_c.cpk::skill/173.pac` | 1 | `2bfa228994b60a4311bd899d69f82c1100d2102799925fad6b3d06507965cb43` |
| `data_c.cpk::skill/174.pac` | 1 | `6822c3e372dcf1013138ae896c0d887506de2900cc89dfc587211ffed7c3623f` |
| `data_c.cpk::skill/175.pac` | 1 | `21d23cc876132fad7b9270bb875e0cc316792b7ff24071a86a543b4c2e7c9c0e` |
| `data_c.cpk::skill/176.pac` | 1 | `25d0819f21cd72bcd8a929cf94035258fcb45566aff0a11fdd558d8974866ecf` |
| `data_c.cpk::skill/177.pac` | 1 | `e336b9d5bccdb9d742bf3e8757a6a36372428501940c9c929306c24d17c4ba9e` |
| `data_c.cpk::skill/18E.pac` | 1 | `f0e378c602a8a63bb73ecaafaf6ede50672be12f3eacbc46d3b5d6cbdd0b2cec` |
| `data_c.cpk::skill/18F.pac` | 1 | `48c3a588e2d951252303bb098bfaff023881651b2dfc8d5bae8a087a63066359` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin` | 49 | `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin` | 49 | `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| `data_ck.cpk::battle/boss/e200.bin` | 8 | `063f67e2b8d3cc5767e36193c74e20ca22a27217abf41ad1f4d8477e9af69896` |
| `data_ck.cpk::battle/boss/e201.bin` | 7 | `269de07c7e6d69f132c5033c321028b51631068cdf84ba4a02910dabd21cf6fd` |
| `data_ck.cpk::battle/boss/e202.bin` | 7 | `7006004f4353017791554ca55ec6c357321829fbb5d267cf5f3b46de3fa1898a` |
| `data_ck.cpk::battle/boss/e203.bin` | 8 | `5fe02edbabe07fa5bb60c2a486f0947882b9c01098d792e1981f94ca0d99a579` |
| `data_ck.cpk::battle/boss/e204.bin` | 8 | `8964d94cd69526ac2bb7ce03569338c19c4384af0abaec5db020a033d98ed4f5` |
| `data_ck.cpk::battle/boss/e205.bin` | 12 | `216a8ad6a21c345c0ec740a428aeabf77e6bfb9085d2ac7ec4e61e326f9928e0` |
| `data_ck.cpk::battle/boss/e206.bin` | 12 | `f0fc64ce9bb95e3006cbf8b4d3ee264dd3631d7c5d6ab4fe73d771e56bedbc02` |
| `data_ck.cpk::battle/boss/e207.bin` | 7 | `962fb97e1d38088b7a62fd005a70c557c134689d241a17766bb118dd09d3ef02` |
| `data_ck.cpk::battle/boss/e208.bin` | 12 | `6e99cf8bd2886dce9f9e6299425beafa57050b6ded5b48d2ce85c75728c192ed` |
| `data_ck.cpk::battle/boss/e209.bin` | 7 | `1eb84458369cd352503565c250b62f4560ba9624e86d352dcd9c9e4d5bc52722` |
| `data_ck.cpk::battle/boss/e20A.bin` | 11 | `e2af9e89de3ea74c380f885249471b4de5d4f442472680b7a3e8eadb6667732d` |
| `data_ck.cpk::battle/boss/e20B.bin` | 13 | `015a4d1ae32c286285f7800431286e29a90c3d531375f5e939e68beac6d292be` |
| `data_ck.cpk::battle/boss/e20C.bin` | 7 | `e9175625bfa8990ea726e224900d5bb79c484d373acb08327dd9717c1e807798` |
| `data_ck.cpk::battle/boss/e20D.bin` | 7 | `e89ed2fa6c546b7e6b2695125149a07e36703e9697093fa643ecafed67e48a14` |
| `data_ck.cpk::battle/boss/e214.bin` | 11 | `d7fd184c7ba331b6cb8dfa38ccbfce90874f35d595115b705224c2063b55b82d` |
| `data_ck.cpk::battle/boss/e215.bin` | 11 | `f92efa92d4aead04bf87f414fc6aede37c38520387c084c4987c1e4847ab3e53` |
| `data_ck.cpk::battle/boss/e216.bin` | 8 | `1a604fb9dce5d44b8e819af6f8ce27ed19eb82e47a975c9f78bda86636d85261` |
| `data_ck.cpk::battle/boss/e217.bin` | 7 | `a1256f72f6baf81f600ef6f756423c44e844c0f313717c877ba07720f5788b07` |
| `data_ck.cpk::battle/result/shuffle.arc` | 8 | `c49183cb1d075f179aac47e9fdfcced53799f24d54c268933882df221e46a76c` |
| `data_ck.cpk::event/fishing.bin` | 12 | `34e49751bad5aca4cba11db75b47e689b0ee3cbeb5539929ecc508e82ecb82a6` |
| `data_ck.cpk::event_data/community.bin` | 11 | `3225dc1a4303c0a9698b91a40461486655ea520c50e09eab64fa0b02723ff3b9` |
| `data_ck.cpk::facility/cmbroot.arc` | 3 | `6525e00b09b3cf8c10880af1e53515d03c280896d818769c73f0b759391b0fee` |
| `data_ck.cpk::field/pack/fd031_001.arc` | 2 | `8cbf45240bc2bf7ba3ae6e34711f086765332f0d7ca6b83b8987fd5636227aab` |
| `data_ck.cpk::field/pack/fd031_002.arc` | 2 | `de3ce9d68953049388f40b169d03a3024b0242b6e227788d609c7c8829a1fda9` |
| `data_ck.cpk::field/pack/fd069_001.arc` | 2 | `56cb705e07c847564b05dbd991b9411d8b5e4281343e7f9c98f2a01d733b14d4` |
| `data_ck.cpk::field/pack/fd069_002.arc` | 2 | `adc1abf6fae422176a0632440c7f02c2dd9344b3e4e325aaa8c8a3cd73e3bc94` |
| `data_ck.cpk::field/pack/fd069_003.arc` | 2 | `bedb2ac65ee3a8f802fe04bf9d6f30be13013657d967328519460b3aba05142d` |
| `data_ck.cpk::field/pack/fd069_004.arc` | 2 | `0c3c4c75f82128748510117da910af2a10369a7657233d2e00ec091ae32ae9df` |
| `data_ck.cpk::init.bin` | 12 | `cc2b9206dffc6b79fb38dfbca88d8578423fce9d2e10c6651d11ce7c97f0e2c1` |
| `data_ck.cpk::init.bin::event/icon.bin` | 26 | `7799e841994b2688152ccb5a8622b104ded5903fd53cc48509d7841ce36a9b16` |
| `data_ck.cpk::init.bin::insect/insect.bin` | 1 | `348d03c29eba568b87f1a7d4cce688a9b32ad08bc67280652111fb389ea8e883` |
| `data_ck.cpk::init_free.bin` | 1 | `65bc30e9554ae746ffde5ba9a4b359559cf017fa0351d4bb604c1b55ae7be143` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin` | 49 | `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin` | 49 | `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| `data_de.cpk::battle/boss/e200.bin` | 8 | `03e35c7d902418660c7b6586915735811dbe3b49b81727bc04abce3649e54090` |
| `data_de.cpk::battle/boss/e201.bin` | 7 | `cb8184d82113f94a25e8e5510c4e8ebf7700e08db26b6a32f8191064107a3c83` |
| `data_de.cpk::battle/boss/e202.bin` | 7 | `22fd4c349da5db31bf9b817179d6375d8d228ccf786fca86e8e8ad101a83a2c1` |
| `data_de.cpk::battle/boss/e203.bin` | 8 | `8d3efc60f5c9324e1ff148527ef8d9e6db9894935a09d0611a95578632d66b82` |
| `data_de.cpk::battle/boss/e204.bin` | 8 | `415eaba4de435b20d7dc7cebb7dd4848d43aa1cb20f23ab865d58a4fb08e214d` |
| `data_de.cpk::battle/boss/e205.bin` | 12 | `d6f1f18523d10cb091fd5b313431c32411e88de0483f4224dc78db6545810103` |
| `data_de.cpk::battle/boss/e206.bin` | 12 | `247a3a5b3baab130e77acda632926c71bd8d5f64bc9d5014c31f3c9655e743ad` |
| `data_de.cpk::battle/boss/e207.bin` | 7 | `ce25f8a0b404781af55102dd090c071ee301e5278d971dce3b0b274a195cf48d` |
| `data_de.cpk::battle/boss/e208.bin` | 12 | `02daec38e9af5a83c812e1f29acde4f087a570955ee2bedbf66d4a026226045e` |
| `data_de.cpk::battle/boss/e209.bin` | 7 | `401abb381739d62269d26c44b311b7a1c2ef58b3f01a9dc4d9190232857bb23a` |
| `data_de.cpk::battle/boss/e20A.bin` | 11 | `e506de9c523a035e82e7a864e8e7692933be9b14babd366762aeb75574f774c3` |
| `data_de.cpk::battle/boss/e20B.bin` | 13 | `9af8473b44776c8a62ffe795c47ed39c2e11163daacb01aa57f97c038585d776` |
| `data_de.cpk::battle/boss/e20C.bin` | 7 | `b1a7ef160d2fb34cb6f46351fcc7f5b16411f07d8bcc79f32fb6a3aa3e88f0c9` |
| `data_de.cpk::battle/boss/e20D.bin` | 7 | `b4a05492c887a163a5a04233ca4c33e233f0469134cc93963cba9d4e2d8d25fe` |
| `data_de.cpk::battle/boss/e214.bin` | 11 | `5b30f09a609d0da6ddbc6f61854ef46afea5273b2a62d04d8b25ba288191bc9e` |
| `data_de.cpk::battle/boss/e215.bin` | 11 | `bd860ee473dba14da1c3c1adbe83dadb083835e9dd62a7f4707ad292618d0a22` |
| `data_de.cpk::battle/boss/e216.bin` | 8 | `1c860b92e4d7024f034f122483e246e8c248b439f118c9d376f514b765e33d8a` |
| `data_de.cpk::battle/boss/e217.bin` | 7 | `c59ebab89f932be3519f8685052226cf5baaa3a1cae23cfb8d75e3b01ac7b823` |
| `data_de.cpk::battle/result/shuffle.arc` | 8 | `0fd728912a0628b64b01a6bbd088c24f717bc696082eff1a6f50a12c1e4391ae` |
| `data_de.cpk::event/fishing.bin` | 12 | `de84d6b4cae4b9b9f73d4eab140dca011f975d408d825bcacebaff9f21e34ba3` |
| `data_de.cpk::event_data/community.bin` | 11 | `c21e73a8b16a7a6b104416b49bb8930332e898365083ba75bbc741d0390c0dce` |
| `data_de.cpk::facility/cmbroot.arc` | 3 | `dbab1c5437e854efed2989361cf30880e9e8cf61fe5a44a28d2edda5b1596503` |
| `data_de.cpk::field/pack/fd031_001.arc` | 2 | `9afa3ab5c66c29ef0da6722f57c14ee5eee6e06b5ad388ed6866394b4147c221` |
| `data_de.cpk::field/pack/fd031_002.arc` | 2 | `f3e3bf50489d10bbf82cb89c8e383d9383629481db76bb654f7fd7acb2368d0c` |
| `data_de.cpk::field/pack/fd069_001.arc` | 2 | `37cb98b1dd3277171bcbc2c00094943880bcb1b2f6177c45d8859474cf3513eb` |
| `data_de.cpk::field/pack/fd069_002.arc` | 2 | `277fceaa57e6b4248cbd1aa2edb686cd0af606a664d4c982b7a8b132abc5809a` |
| `data_de.cpk::field/pack/fd069_003.arc` | 2 | `0cc4127f0278f32ede7a7c1e78334bbcb5518afe1f77b6715bfc583792fa6eee` |
| `data_de.cpk::field/pack/fd069_004.arc` | 2 | `8782a833bb23620c6f9645633d5025a99933168ffbdde1a27fdea17c4e2f92f9` |
| `data_de.cpk::init.bin` | 12 | `4bb9773826779e8a563d1c9d18970f254f4174c785551df9649316e5c0695c38` |
| `data_de.cpk::init.bin::event/icon.bin` | 26 | `7799e841994b2688152ccb5a8622b104ded5903fd53cc48509d7841ce36a9b16` |
| `data_de.cpk::init.bin::insect/insect.bin` | 1 | `348d03c29eba568b87f1a7d4cce688a9b32ad08bc67280652111fb389ea8e883` |
| `data_de.cpk::init_free.bin` | 1 | `66a4674a4181a3006439982315a395af7d72457c54d7c48d8b5f7093cc46b526` |
| `data_e.cpk` | 15 | `620741aca9601ae2d48a391b87b3064c2e778b8f47833a360430cf0e55eeb8c1` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin` | 49 | `6f45c578705edee35906af74421c7c15f4916460a4a97915a3350fef10cdf0af` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin` | 49 | `6f45c578705edee35906af74421c7c15f4916460a4a97915a3350fef10cdf0af` |
| `data_e.cpk::battle/boss/e200.bin` | 8 | `8b7aacd51aea89ce8327f6e870a0b0113f7c3726f8ef78cfdc3173ab0707cd13` |
| `data_e.cpk::battle/boss/e201.bin` | 7 | `d4db3ea29f0e1f74f0eb4a1700ec3e06bd5a0c91fbea81cfbfdd88cb838b5c89` |
| `data_e.cpk::battle/boss/e202.bin` | 7 | `2ce75af9c831960c3aed1098f7216aa50c7546224048a296c3d2798160a7d01d` |
| `data_e.cpk::battle/boss/e203.bin` | 8 | `1f66fada9226fadb3600e951c3466e49156e9a1813c47d522e22f8eec4980898` |
| `data_e.cpk::battle/boss/e204.bin` | 8 | `e7de634225b45b5b16f0d09f3419ab15b94b4390860199acdaa6ed9216f916cd` |
| `data_e.cpk::battle/boss/e205.bin` | 12 | `4c20a216f4abda5b224f7ac630540f482c6e0d1a1769dbb63e7c3d565dc251fa` |
| `data_e.cpk::battle/boss/e206.bin` | 12 | `513238723730d2b251e0a18c8d9bb24b458d874038fdadca1169977d8f3ac9ad` |
| `data_e.cpk::battle/boss/e207.bin` | 7 | `5cfb4dfa992f5e16a3fa786e8d7ce3f3904cf860459bb8697a5820faafd3b2ad` |
| `data_e.cpk::battle/boss/e208.bin` | 12 | `c5ba012181972e92001ecdb5a9257f69a937b085f040014dbe824aaa23290d28` |
| `data_e.cpk::battle/boss/e209.bin` | 7 | `5c22d957e8bd47a18be6159de7949338ee80d145d33fd99a787b4fde95ae54b4` |
| `data_e.cpk::battle/boss/e20A.bin` | 11 | `430ae2bd368be09a2b972c5782a88ecf80bb6ec90be74a2d44c27909001af133` |
| `data_e.cpk::battle/boss/e20B.bin` | 13 | `49235de213c53b5dcc68cbb4cdfe1834e530699ab2b89cc8dff639f3acd01c4c` |
| `data_e.cpk::battle/boss/e20C.bin` | 7 | `a1733949f91d42fb00ad20bc4118eefe28ff8983ea8fa341848639d34b987aa8` |
| `data_e.cpk::battle/boss/e20D.bin` | 7 | `ffd768ee925bc5761dc9527258a5df49ad0f7fcd147345b67a11f254989b9fd4` |
| `data_e.cpk::battle/boss/e214.bin` | 11 | `6f79e23f984f9fbf5bbcab45cf0c1395c41ec0042adc8926e5058770027b321d` |
| `data_e.cpk::battle/boss/e215.bin` | 11 | `148fabfbd63523f94bbe4a0df5dca5a7907e139dfdd005d4206a26572fd79a82` |
| `data_e.cpk::battle/boss/e216.bin` | 8 | `07762e4189c4de9649b486a75e55d8d197e52f1740f8cd78ce99a47618748a72` |
| `data_e.cpk::battle/boss/e217.bin` | 7 | `0dfd0ac5565b4e16caf5282ee67e26a56962037d922af600e911b2216a858ca3` |
| `data_e.cpk::battle/btleff.bin` | 49 | `6f45c578705edee35906af74421c7c15f4916460a4a97915a3350fef10cdf0af` |
| `data_e.cpk::battle/result/shuffle.arc` | 8 | `a7a99fb54f33fbd984ef3e2cb33f278d41e6a68bf487b014a8df34c7c81ddc03` |
| `data_e.cpk::event/fishing.bin` | 12 | `57d2b8d853e47cf58b3eb05cb6257b6cdded35946aff283fec0ee24dbefe15d1` |
| `data_e.cpk::event_data/community.bin` | 11 | `21ed73e670ab083f1cd78913aede412195a253896173d1894c0c9fcf637eac92` |
| `data_e.cpk::facility/cmbroot.arc` | 3 | `0f8893f3029132e51e8c9df1e97fd886acbc890d367f8e26413d029efb725049` |
| `data_e.cpk::field/pack/fd031_001.arc` | 2 | `2080e83923295c80c24de5afcd93d262812079731763ff51be93cb45d9bd4448` |
| `data_e.cpk::field/pack/fd031_002.arc` | 2 | `bbac5a07b911a5746bd1c1aceb1d08348fe977358105bccecb0224f06590722c` |
| `data_e.cpk::field/pack/fd069_001.arc` | 2 | `0e009843e55cfc6d2118986b074abe6013c62b84d57341d6b7ede55e9e807563` |
| `data_e.cpk::field/pack/fd069_002.arc` | 2 | `ed34f4e01323ff708cdb9dee8b2c12d8214e81c18fb73a60fb508810cc8f0dfa` |
| `data_e.cpk::field/pack/fd069_003.arc` | 2 | `482f18c0e0d5522e7b9520ec00e33cc4323a35e19cc11458035cc116490d9c98` |
| `data_e.cpk::field/pack/fd069_004.arc` | 2 | `3466125b48d3d2a5151f4057521e45bd226a4fadbd9e5eb4ef04800ea9609e62` |
| `data_e.cpk::init.bin` | 12 | `53fb124e9101917c58520fc234addc57cb66d1d14f3daa0266a8f91a6c3e2b36` |
| `data_e.cpk::init.bin::event/icon.bin` | 26 | `7799e841994b2688152ccb5a8622b104ded5903fd53cc48509d7841ce36a9b16` |
| `data_e.cpk::init.bin::insect/insect.bin` | 1 | `348d03c29eba568b87f1a7d4cce688a9b32ad08bc67280652111fb389ea8e883` |
| `data_e.cpk::init_free.bin` | 1 | `4351ea2508eb58dde94338224848b36f96d68a56c68df55c9a1545b78eaa9c2b` |
| `data_e.cpk::skill/100.pac` | 1 | `ebfbe2cb2a43dd5d02ee35aeacf935364a7a91aef7823f6eb1d99a34888c77f2` |
| `data_e.cpk::skill/101.pac` | 1 | `fc3aeca253b4ac9fe591419667640bb1b3e50b4c0ac8f229e3cdf7a692f50fc8` |
| `data_e.cpk::skill/102.pac` | 1 | `8cdce211a2e77edc6f77b4516782fd6dd7a2d444af0a785e87af039b652b1739` |
| `data_e.cpk::skill/11C.pac` | 1 | `07d5bd544151641086584caa0b0cc27e6a653d910ef355c095cf9454bdc43f61` |
| `data_e.cpk::skill/172.pac` | 1 | `b0f0288ba2a0eb19116fbdeb66bd274de5081e4ead456ab5ba04493afd53528a` |
| `data_e.cpk::skill/173.pac` | 1 | `36d48839288c00e3347f4178ed9ad1ac8f88c81e6e3402ca3c363e20f19562b9` |
| `data_e.cpk::skill/174.pac` | 1 | `737d7c1e7b7a1077fd84e14787f63aa9c4f3500986bfacac9d78ce98654e88f2` |
| `data_e.cpk::skill/175.pac` | 1 | `d62b668b5696ec47a9a110d098bb4a214465db9b0aa7c55f6c45c9e94dca53ee` |
| `data_e.cpk::skill/176.pac` | 1 | `397f00822369a5884e3ecb950008695325ebfb7471963752b7a33e7b8cf1dfab` |
| `data_e.cpk::skill/177.pac` | 1 | `a214d66181c9eb72cddd73306b961764ff36064178ed6c5b7d7d8df20ced7866` |
| `data_e.cpk::skill/18E.pac` | 1 | `d63a5bf9cd6478947cec31e8cbc6a1dd97dd7880868c0cac3ecd51597cdaf4f3` |
| `data_e.cpk::skill/18F.pac` | 1 | `1c1d762d1c065354ec2d2bbda57309b865f913ea36df74a58ddee381dd597265` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin` | 49 | `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin` | 49 | `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| `data_es.cpk::battle/boss/e200.bin` | 8 | `986b4c901d16a9fcdf5678b4948781e2fc0c983f8313b30aadecd59d55717c4b` |
| `data_es.cpk::battle/boss/e201.bin` | 7 | `312d316ec9fe8d227370488a6c6fef474c4839613954ca0f0b3ec9c68711a252` |
| `data_es.cpk::battle/boss/e202.bin` | 7 | `f690b9f74d5ca527c5217dfcab0a0465befd4fe7e73871fef58d673dc957b572` |
| `data_es.cpk::battle/boss/e203.bin` | 8 | `ce66e7b990a75cc8c0e1c4ffc600e3debc9099fa848241fdfca219e0ffef811e` |
| `data_es.cpk::battle/boss/e204.bin` | 8 | `d161023ac5ac087303bb8451e6b3f24e8601b41211a7758ef2b3e7a6fb3aae5c` |
| `data_es.cpk::battle/boss/e205.bin` | 12 | `84f711cb2d7b5dea2da0e7cf024af7f9357b2794a522aeee44be555a285478af` |
| `data_es.cpk::battle/boss/e206.bin` | 12 | `412b69b59359bc18cd2b95970c616862c7104ffea6da1d25f1694e5f00058133` |
| `data_es.cpk::battle/boss/e207.bin` | 7 | `2a38efaecac6224f96e02b20a7d4c1e887e1234338ae4780ddb1758f6768604b` |
| `data_es.cpk::battle/boss/e208.bin` | 12 | `63b0dc6a19f1afbaeeb92fd5b8f15784619d0225545c335ace4b290d4634a8d6` |
| `data_es.cpk::battle/boss/e209.bin` | 7 | `b56be4876a38fe438f9c6919e23c8491d7273cc80d5f1dcb501d9b198a68d00d` |
| `data_es.cpk::battle/boss/e20A.bin` | 11 | `3c20f80cecd4fb8c887b4efeabfd813657b5d781fd5ff231ab3309577437d3a5` |
| `data_es.cpk::battle/boss/e20B.bin` | 13 | `0543e1cb5d6e1f9b4c6607792937119019b1b7ae1ea00f7ccb905e1f379003b8` |
| `data_es.cpk::battle/boss/e20C.bin` | 7 | `f3fb79493f6da55d2523a2e57cda58a5285ec694cc58aad18ffff3f7610ad9c9` |
| `data_es.cpk::battle/boss/e20D.bin` | 7 | `b0182742d9d947f4ef8f7be05a9e4bcd946129e2fefa97a715ef49317059f3b8` |
| `data_es.cpk::battle/boss/e214.bin` | 11 | `5b4d3649eeabe0f268fb4c477653b167cac12a76b6f23e9007a3ddf75789f7ac` |
| `data_es.cpk::battle/boss/e215.bin` | 11 | `c514137279ef5e1831972be8def10645331674bf5dfd0b8462bfb778cc303d0a` |
| `data_es.cpk::battle/boss/e216.bin` | 8 | `5f2602bb1316c1e6b23ddaa294958985d79315e47115f0391978ab999673b170` |
| `data_es.cpk::battle/boss/e217.bin` | 7 | `3478932b2d67ae5380c5850d0a886b1eb52623b99bb16717a1044e5bd7230d83` |
| `data_es.cpk::battle/result/shuffle.arc` | 8 | `6ca903f5d439135c3ee43e67d52c4acb9083620e92d4c514bb695ce75162a043` |
| `data_es.cpk::event/fishing.bin` | 12 | `13b311efcdf9f6341b21cd6672fef6f365bca07407b9a67cfda7580448bba3fe` |
| `data_es.cpk::event_data/community.bin` | 11 | `98491cb05f3b3055cb0a32be820d35337635821f1c25f00e56b5f62ee7865660` |
| `data_es.cpk::facility/cmbroot.arc` | 3 | `c3f41b271e1b3023d27f57702a433c9425fc989a3e8c49a6ddd9149c7b06f0f6` |
| `data_es.cpk::field/pack/fd031_001.arc` | 2 | `b83bbcef3bf0e9cc98147cbf568d9d147d5b871c37156d7938b08e83f1eae3de` |
| `data_es.cpk::field/pack/fd031_002.arc` | 2 | `584e1eb676cc4fb8829d9bb9dd6194c7360a31214fa5bacac690383a687d28c3` |
| `data_es.cpk::field/pack/fd069_001.arc` | 2 | `3c343e88ae605e2b2faf2cd4ed46dec71dc9e731fb490cd5d538790afc82ab8f` |
| `data_es.cpk::field/pack/fd069_002.arc` | 2 | `4627b88e4ec9d2cad4d8cea0f016ad7e33d2628bd41ec6c3e11849c27df993b3` |
| `data_es.cpk::field/pack/fd069_003.arc` | 2 | `e503bccce980958278816b8e62f8077f9a67a42c28159977fb86e7cac459f3e3` |
| `data_es.cpk::field/pack/fd069_004.arc` | 2 | `cb8d8f8e3ed44f79849f3847be75c07f1aad5944e21aed14bb4ac89600eaa0bb` |
| `data_es.cpk::init.bin` | 12 | `550be5ff99a0f4d31f488f87d6f6e391ceee4b209ad648fa3a589716cb9905aa` |
| `data_es.cpk::init.bin::event/icon.bin` | 26 | `7799e841994b2688152ccb5a8622b104ded5903fd53cc48509d7841ce36a9b16` |
| `data_es.cpk::init.bin::insect/insect.bin` | 1 | `348d03c29eba568b87f1a7d4cce688a9b32ad08bc67280652111fb389ea8e883` |
| `data_es.cpk::init_free.bin` | 1 | `8de8be49e1e595cf9748a0e1840cd0a39e75e3bbafa5b2ef4b37b284dab8e292` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin` | 49 | `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin` | 49 | `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| `data_fr.cpk::battle/boss/e200.bin` | 8 | `69768e813068e24cba0e776cc6624058dd94c8a509450119ae3d5646b000a2a0` |
| `data_fr.cpk::battle/boss/e201.bin` | 7 | `35641c80af328d04a3412e04b477942e78fc1fee4773c603252c6e10c98f33d6` |
| `data_fr.cpk::battle/boss/e202.bin` | 7 | `f3166670caa051d63405b831a475a29c1f27989a880de8c1f2c58d84c14172ad` |
| `data_fr.cpk::battle/boss/e203.bin` | 8 | `c9cc510984c240bf626f527d4ede93868c87536b697a8f71dacde067c182f000` |
| `data_fr.cpk::battle/boss/e204.bin` | 8 | `67ff301fc2af69b8b278e886107ff5edc8bbfcfbfa0b4a47f7673a69e10ddafe` |
| `data_fr.cpk::battle/boss/e205.bin` | 12 | `3a236d3e56260925f8916c9acc52d79a0df5ab8972b105a871e12daf114aa455` |
| `data_fr.cpk::battle/boss/e206.bin` | 12 | `d93dae27825988ea0035c834b6ee814dc35eb173b46f888780973efa2e056c9e` |
| `data_fr.cpk::battle/boss/e207.bin` | 7 | `6a7e01ab3dfe6c65f25862858cdb6c5099e81d11496b4743b3e2dc2719ad5c2a` |
| `data_fr.cpk::battle/boss/e208.bin` | 12 | `51380435fc475b49facdf963d411fab34f877a4e5ec44c6d5132c9add7a0289e` |
| `data_fr.cpk::battle/boss/e209.bin` | 7 | `db0b6ce3f36bb5b2b9ed5bc3e29b099252c83a1630ec4e1d042804a222dfcb5e` |
| `data_fr.cpk::battle/boss/e20A.bin` | 11 | `fc51390bbd3151975fd1536e97596d022b7240246b01f33b572f450b744617a9` |
| `data_fr.cpk::battle/boss/e20B.bin` | 13 | `4f54b024841756f0424c50347836f9ef96420ca9ec2e365a8abbe36e42de5769` |
| `data_fr.cpk::battle/boss/e20C.bin` | 7 | `8dacea33a500f179e8497473e1ada486133fef8d843cfb6fc345be9d4b8e9f96` |
| `data_fr.cpk::battle/boss/e20D.bin` | 7 | `beac66e399f26a5ba641692df06c327c0863e42832b73cecf14d3b3c453dcfe7` |
| `data_fr.cpk::battle/boss/e214.bin` | 11 | `8170ba7167377dc6d69da7a4a85a23623bf7ba6b45207ad51b42243ef024dddf` |
| `data_fr.cpk::battle/boss/e215.bin` | 11 | `b680579b6cca3a74b5373d10f1d6ec3c000219a9cdb5e50fabc0ee99c30b7f23` |
| `data_fr.cpk::battle/boss/e216.bin` | 8 | `e4a3f62e4cfb4a846e184b78c299d3078d01f86126c5f1aa9dfe12e2e1fbdc28` |
| `data_fr.cpk::battle/boss/e217.bin` | 7 | `42b1aaeda9fbcc26f75dd4926a0403d52e8512f2f3671c5e232158d24f6decf8` |
| `data_fr.cpk::battle/result/shuffle.arc` | 8 | `0e7ebbf9ad3d2d5063bf244a6e8f51ad8636856f9afe5a92dcc02c864831718c` |
| `data_fr.cpk::event/fishing.bin` | 12 | `009bd9953ca3c527058c7cda0ec3921ffccb6f02185f715533ebcb0cfe89c239` |
| `data_fr.cpk::event_data/community.bin` | 11 | `899a16fd45fdbdb199089d6d9a08767eaae5cd8580650d9017e142e01fda169c` |
| `data_fr.cpk::facility/cmbroot.arc` | 3 | `0c16ee49598637aeced43173297ac30deddf2ee5573253a4b7eebbf6af46eafa` |
| `data_fr.cpk::field/pack/fd031_001.arc` | 2 | `7b5578cdf89d5545a1b2910a2dfed412a66d0b33635bffc3e1ddc886fbc39268` |
| `data_fr.cpk::field/pack/fd031_002.arc` | 2 | `0369b6382c03f82748139c7bc9c6d6d435bc19536b576b225ea66f1fa7ef7d6d` |
| `data_fr.cpk::field/pack/fd069_001.arc` | 2 | `355d94e33c2cfa49ea95526961cc605d28c91872382b15bcb3f0a97976671dde` |
| `data_fr.cpk::field/pack/fd069_002.arc` | 2 | `03d3ea194c2591ee8a7531e764555aed0d4b688a3b17f3833dc39b1342393d50` |
| `data_fr.cpk::field/pack/fd069_003.arc` | 2 | `eb6ba111ac7943f8244a53743568bb4856da6345c48ea77607d4d16ce6f31501` |
| `data_fr.cpk::field/pack/fd069_004.arc` | 2 | `46006c0044915e18aa88230be41e40b1ed60d571356df0ba74c415dd99d1ea33` |
| `data_fr.cpk::init.bin` | 12 | `248155997380a74db3923933778ab1e03b546f734fce507f9bb3e10dd2998402` |
| `data_fr.cpk::init.bin::event/icon.bin` | 26 | `7799e841994b2688152ccb5a8622b104ded5903fd53cc48509d7841ce36a9b16` |
| `data_fr.cpk::init.bin::insect/insect.bin` | 1 | `348d03c29eba568b87f1a7d4cce688a9b32ad08bc67280652111fb389ea8e883` |
| `data_fr.cpk::init_free.bin` | 1 | `c77c257830b7b20a57e33e5df9727c6adda71d9943c76580529b65ea38f332b4` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin` | 49 | `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin` | 49 | `96f82ee716f7f947c82a980b480bec1dbf08c4dd4958e3fd9105d56c97ca53d7` |
| `data_it.cpk::battle/boss/e200.bin` | 8 | `c3a18eee378d4d542ea5af396e8ca715c19566a10213e01e1d0051ec91f3718a` |
| `data_it.cpk::battle/boss/e201.bin` | 7 | `40a06fe9ed603baac9df9fcb5a422b7c189fcad655fd8a65a9ec8678a539931f` |
| `data_it.cpk::battle/boss/e202.bin` | 7 | `1bf19fffc457e2f83e1a6f3edb3786d9c3299d91fac94e09cab63521643befd5` |
| `data_it.cpk::battle/boss/e203.bin` | 8 | `5b99476e1f1558ded98846770ad3cd244627bf0e7a7f1388e514628c2c39ab9b` |
| `data_it.cpk::battle/boss/e204.bin` | 8 | `c2d882b5494122a6d9841410047016b67cdffa26ff42cf10baa5c3341133391e` |
| `data_it.cpk::battle/boss/e205.bin` | 12 | `5f933e5b48c7c5b7db475529e2ca5d5237a0be65d306849b774c44c171924c27` |
| `data_it.cpk::battle/boss/e206.bin` | 12 | `229ae10d71151590ae12266f0d517722677cffd27e7448945e6e529dd4287fbc` |
| `data_it.cpk::battle/boss/e207.bin` | 7 | `ae157860a38df45297aa00c7b0ffc1d122cfb755d1beaebd311c51dfc7adabb8` |
| `data_it.cpk::battle/boss/e208.bin` | 12 | `326054f651849b1021a9d04b620ac8b35037a14574ba95926fb478758e776c93` |
| `data_it.cpk::battle/boss/e209.bin` | 7 | `301b5e4147f97f37f40038de07d40b0f5bd2e48830c7c7dfea5b73a45763a0d4` |
| `data_it.cpk::battle/boss/e20A.bin` | 11 | `c3ec6a6846e32d2f46e1daffff15686c4287062875f81a6312ca6777e5bfc543` |
| `data_it.cpk::battle/boss/e20B.bin` | 13 | `0f86aa19fa5e9430831033342e0fa60da857d306cc9250fc48e204527b4ac4ec` |
| `data_it.cpk::battle/boss/e20C.bin` | 7 | `1c3e03c58b795b81d867989162899ade6cab83e912d23191f27a805c32064f21` |
| `data_it.cpk::battle/boss/e20D.bin` | 7 | `2edf5da695c35d2983ae6484b99258473435c08f4f8087dd49daf14f5c68b211` |
| `data_it.cpk::battle/boss/e214.bin` | 11 | `4aee5fd24ab10114eff0739d5bed643c4561db4fcb8f79864a0f4a52902f36bf` |
| `data_it.cpk::battle/boss/e215.bin` | 11 | `1ac87f45ad953392811a8c14f026df4a7dca07469a347df5983041cf02f3c365` |
| `data_it.cpk::battle/boss/e216.bin` | 8 | `c614c37b8251338241ec85a2331cb5940e1e7f72392eff516d5e84dddbaa7b96` |
| `data_it.cpk::battle/boss/e217.bin` | 7 | `ad4f92a6b86e639086a2572f5b6844bf32b671abbe90530efb9015d774145b2b` |
| `data_it.cpk::battle/result/shuffle.arc` | 8 | `198c4e3d1d9bc83b4c5b535bd25d87d7ba5aa8fd79eeeb1debf5cfb4dce0e447` |
| `data_it.cpk::event/fishing.bin` | 12 | `a0dcb3228574d2a5ab4430450d37984ea5cbf341a286175c2ceae3b4e816bcdb` |
| `data_it.cpk::event_data/community.bin` | 11 | `6076969b39b044b6be208e3f41990f7a1a80ac6f296c938bf068f0d0636defec` |
| `data_it.cpk::facility/cmbroot.arc` | 3 | `32c22f1b0c5b8e37a8092e6969a019cdb7d610ac5086809e0ca39be1b1872b6e` |
| `data_it.cpk::field/pack/fd031_001.arc` | 2 | `1f969587a68fe98bfc6889c78787bde5ec13aad6527b7308fdcad5e06c52dfbf` |
| `data_it.cpk::field/pack/fd031_002.arc` | 2 | `5c9b53c7259de40e5bfddb4a410809fd81f1bc4584933895db7ea4a0ba799a98` |
| `data_it.cpk::field/pack/fd069_001.arc` | 2 | `be5439701fadb76f31bbad0c313056481774e19c69478f1f215c1061e79d5e40` |
| `data_it.cpk::field/pack/fd069_002.arc` | 2 | `d55d6f6284b8fefb83da0fba3a028858f00027faccf576fc17bb2396de32c7a0` |
| `data_it.cpk::field/pack/fd069_003.arc` | 2 | `2bd4b99265e360e0a24d3ab8fb4fa79970b83d24b654570385cfef4c5018eb80` |
| `data_it.cpk::field/pack/fd069_004.arc` | 2 | `ec7342e49878a2faf12783d1d2df8b34e26ff6ac8aa8bf2ecdbb2f62f978094d` |
| `data_it.cpk::init.bin` | 12 | `6b96837c47a710d5cc338ac75ec293bfcc589f67153194777066330922bfe393` |
| `data_it.cpk::init.bin::event/icon.bin` | 26 | `7799e841994b2688152ccb5a8622b104ded5903fd53cc48509d7841ce36a9b16` |
| `data_it.cpk::init.bin::insect/insect.bin` | 1 | `348d03c29eba568b87f1a7d4cce688a9b32ad08bc67280652111fb389ea8e883` |
| `data_it.cpk::init_free.bin` | 1 | `5274b0c9e535cdcf9d8b593d6ef0c3734bf274890afef624efa36b8cd570b102` |
| `data_k.cpk` | 15 | `15956365ab57abad1251b74c2907b2d10f894baad2829cfce6ad0ef741893d9b` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin` | 49 | `da6092125a03d4c9235cb722ddd815c4c0a573f4634667369ade1db9393f0223` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin` | 49 | `da6092125a03d4c9235cb722ddd815c4c0a573f4634667369ade1db9393f0223` |
| `data_k.cpk::battle/boss/e200.bin` | 8 | `44b70e1b0b7de2d0aed265682eeed1b18d4a07d666cb94a77d57eba030d8858a` |
| `data_k.cpk::battle/boss/e201.bin` | 7 | `2428b81eef57d18d955e67eeafd8ac855ed0c14500075d38341eaa35505f5df2` |
| `data_k.cpk::battle/boss/e202.bin` | 7 | `8f19d8fa04a4ad571ca8d28059c58d8518a439449971b3f7bcab8f8e82e8df90` |
| `data_k.cpk::battle/boss/e203.bin` | 8 | `5451a61fdc004e878987e462fa7b906068edd18466a199551647de311777694b` |
| `data_k.cpk::battle/boss/e204.bin` | 8 | `8c3ebb41f2786e0cc36779ed3083a2712f01d597c85eed38dac6e41f5dc1a7de` |
| `data_k.cpk::battle/boss/e205.bin` | 12 | `fc2edc2383a2e352e9baeecd9d259c5edec82b53ced1c0f0437f1d86c25c2e8c` |
| `data_k.cpk::battle/boss/e206.bin` | 12 | `51da1f755cd325a42a0c5d2a8dcf83b18783154aee922b77af20c8733e422f4a` |
| `data_k.cpk::battle/boss/e207.bin` | 7 | `8db6c00bcd7c9fd1721935a01ff1ff499addca8d8bafa644f4fa4c646035f69b` |
| `data_k.cpk::battle/boss/e208.bin` | 12 | `a8260635fc348a81767bdc98dab822043e1df00d14768b112f4a97ce1f48e633` |
| `data_k.cpk::battle/boss/e209.bin` | 7 | `f40ed4c64eb9b3eeab82597f24218433a9037254374bdff564fc4d35bc019f1f` |
| `data_k.cpk::battle/boss/e20A.bin` | 11 | `e7dfd36a1abe5b0226cace32a477fb47a046827984b702e26a22606ea27d0fd9` |
| `data_k.cpk::battle/boss/e20B.bin` | 13 | `3ba4a0555ed9ad94a4f1f82269840e0d61e499eead0d33a0cd8b494d515ac971` |
| `data_k.cpk::battle/boss/e20C.bin` | 7 | `75d95fa8361c19cc11a04fbe9283bc07e91eb079a4fa7b579c0cf348de11a802` |
| `data_k.cpk::battle/boss/e20D.bin` | 7 | `62fcc4354e6dbcc6be85e4f1eea17284e5237da7fdb389b274d92ef6c1005859` |
| `data_k.cpk::battle/boss/e214.bin` | 11 | `97a3bcaf16213e1518dd81f91c2268abb9d43c238a7a071f7c165f1892133f83` |
| `data_k.cpk::battle/boss/e215.bin` | 11 | `d7eb9bab754241e2eb2134562e183a1c81252d6e1ac588ff23aa33d3a6a0d23b` |
| `data_k.cpk::battle/boss/e216.bin` | 8 | `7b92a96ff9337c808aeeeab8349226e2a25f32b0dc3fc25483eb629a161b8b58` |
| `data_k.cpk::battle/boss/e217.bin` | 7 | `2789fa74c768352cdc8bcc62de42ab8793557316366ad4f23d907e523e117f75` |
| `data_k.cpk::battle/btleff.bin` | 49 | `da6092125a03d4c9235cb722ddd815c4c0a573f4634667369ade1db9393f0223` |
| `data_k.cpk::battle/result/shuffle.arc` | 8 | `bf776f85bb58fd15356ca18d620c19a86bc9b6bb914946ccb6ecf716b14eebfe` |
| `data_k.cpk::event/fishing.bin` | 12 | `84a53ba8affd14320786257e0cf9656f8b002de1b0a99ff36116c37f6efe6617` |
| `data_k.cpk::event_data/community.bin` | 11 | `751bed52c66d17c7db7c00bdcadc181cc3fea3fc5154569e3c3705f7a7f0f590` |
| `data_k.cpk::facility/cmbroot.arc` | 3 | `daf80508a4740c79cff314ced635210d2b1fec2abc4c0cddd3fea0e1dd6f3725` |
| `data_k.cpk::field/pack/fd031_001.arc` | 2 | `11bca32f32faf74ae0731233374e2f21fdec9e6766adac7032741605a574f2f1` |
| `data_k.cpk::field/pack/fd031_002.arc` | 2 | `87a942436295238567ae4a79f9a3652e056649e2b0a415d5a02b9dd9596f4216` |
| `data_k.cpk::field/pack/fd069_001.arc` | 2 | `7c217d8dfb09bc803b93bc2cb05e4065ea2851ea911afd531402a69c927a59db` |
| `data_k.cpk::field/pack/fd069_002.arc` | 2 | `71f5026f0a522ab90bfc4e11f1126abe88522c0ead558dc3641cc2a894a7d234` |
| `data_k.cpk::field/pack/fd069_003.arc` | 2 | `8944230a597efdd4b984336aadc851625a936b6a81b2286583f807d8f6f4afc3` |
| `data_k.cpk::field/pack/fd069_004.arc` | 2 | `337ad454767ba33754fed5a67e7b9a3e3e94dbcec4fdaa1a0a9cea0bf96946e2` |
| `data_k.cpk::init.bin` | 12 | `96f80f6c2198d8699358c472da4c51d68f4acb90c4d2a8e5045cb66a178f986b` |
| `data_k.cpk::init.bin::event/icon.bin` | 26 | `7799e841994b2688152ccb5a8622b104ded5903fd53cc48509d7841ce36a9b16` |
| `data_k.cpk::init.bin::insect/insect.bin` | 1 | `348d03c29eba568b87f1a7d4cce688a9b32ad08bc67280652111fb389ea8e883` |
| `data_k.cpk::init_free.bin` | 1 | `8e458af6edd3b3c00ca1ed83ed61bae16c4fabb90994f6e98f5c1e3263df89a7` |
| `data_k.cpk::skill/100.pac` | 1 | `de6518e7e7b87ca04d2510d5328f0567391582ec0b28cea92edddd5bea622365` |
| `data_k.cpk::skill/101.pac` | 1 | `7b8112096b54876bfc09bd75924bb966fb6e8983a3c00fd470f104694070bc78` |
| `data_k.cpk::skill/102.pac` | 1 | `82c06525665794fe5f7897bc2469828d96941e74602686ff798eea5f296b5822` |
| `data_k.cpk::skill/11C.pac` | 1 | `949a252e853ecc5c8f5be26993bd3f45b3c1ec521d43cc11438c26c9693f595b` |
| `data_k.cpk::skill/172.pac` | 1 | `b0f0288ba2a0eb19116fbdeb66bd274de5081e4ead456ab5ba04493afd53528a` |
| `data_k.cpk::skill/173.pac` | 1 | `2bfa228994b60a4311bd899d69f82c1100d2102799925fad6b3d06507965cb43` |
| `data_k.cpk::skill/174.pac` | 1 | `6822c3e372dcf1013138ae896c0d887506de2900cc89dfc587211ffed7c3623f` |
| `data_k.cpk::skill/175.pac` | 1 | `21d23cc876132fad7b9270bb875e0cc316792b7ff24071a86a543b4c2e7c9c0e` |
| `data_k.cpk::skill/176.pac` | 1 | `25d0819f21cd72bcd8a929cf94035258fcb45566aff0a11fdd558d8974866ecf` |
| `data_k.cpk::skill/177.pac` | 1 | `e336b9d5bccdb9d742bf3e8757a6a36372428501940c9c929306c24d17c4ba9e` |
| `data_k.cpk::skill/18E.pac` | 1 | `f0e378c602a8a63bb73ecaafaf6ede50672be12f3eacbc46d3b5d6cbdd0b2cec` |
| `data_k.cpk::skill/18F.pac` | 1 | `48c3a588e2d951252303bb098bfaff023881651b2dfc8d5bae8a087a63066359` |
