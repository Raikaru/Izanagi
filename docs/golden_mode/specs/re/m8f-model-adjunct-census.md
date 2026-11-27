# M8f embedded model dependency census

These are descriptor-declared adjunct3 spans and primary27 inline model spans, not extra standalone BED/EPL files. Models use the existing M8c contract pinned at48afcdf5204d388e021dd1f786ab7fef39afb126; source/suffix/backend unknowns are not erased by the classification. The native inline boundary is primary+70, not a magic search.

## Totals

| Family | Logical physical-descriptor spans | Distinct SHA-256 |
|---|---:|---:|
| GMO | 2024 | 369 |
| AMD-CHNK | 245 | 129 |
| legacy-F0F0 | 132 | 13 |
| tool-PIB0 | 2 | 2 |
| inline-GMO | 104 | 24 |

Total adjunct3:2403 spans/513 identities. Inline helper models:104 spans/24 identities; all537 payload hashes are distinct across these two storage families. Thirty-three distinct helper primaries were re-extracted, each with8 zero padding bytes at68..6F and an OMG model at70. The read/size/SHA and bounds pass had zero exceptions,36.90 seconds. Adjunct3 hashes/heads reuse the final full EP traversal; all logical root duplicates retain separate occurrences.

Family labels are signatures, not implied rendered support. AMD-CHNK has ordinary MODEL_DATA member framing from M8c. Legacy-F0F0 and tool-PIB0 remain explicitly compatibility/loader-semantic OPEN; never feed them to the GMO parser just because adjunct kind is3.

## Per-CPK counts and selected-set hashes

Selected-set digest: SHA-256 of sorted compact-JSON rows[path,root-relative offset,family,size,payload SHA], each followed by LF. This is not a CPK whole-file hash; the leaf census pins whole CPK/parent identities.

| CPK | GMO | AMD-CHNK | legacy-F0F0 | tool-PIB0 | inline-GMO | Selected-set SHA-256 |
|---|---:|---:|---:|---:|---:|---|
| data.cpk | 857 | 164 | 116 | 2 | 58 | `1d262957f0e8dd48d667047412c40c44896e317ef3493c74cd62aa3e8c8b3751` |
| data_c.cpk | 202 | 12 | 2 | 0 | 8 | `a1f7bac9104376e231ac5efce30380e76eea41f186abb185cede96a179e388d6` |
| data_ck.cpk | 109 | 5 | 2 | 0 | 4 | `1c5779039f3bdf5a37302e357102689dd7267377da32f87b1a01193bbf729200` |
| data_de.cpk | 109 | 5 | 2 | 0 | 4 | `63b4f376bd4df52a93a83f055fcf307b2a96b9f702c09a4fbb2500d7adf9f24d` |
| data_e.cpk | 210 | 22 | 2 | 0 | 9 | `7784907a2677126d90d51df141cee1eeadddddfeeaab8e59f04762d523f53719` |
| data_es.cpk | 109 | 5 | 2 | 0 | 4 | `fd2020f837eacb65acd5fe4581cf536ea3d725e65fd3becee79354b5b1c593ff` |
| data_fr.cpk | 109 | 5 | 2 | 0 | 4 | `89a7cd84e693829b33fb9925e53f066278dfcba149e27df166626379fe330493` |
| data_it.cpk | 109 | 5 | 2 | 0 | 4 | `6b9ad0eab4ee142b1594e08f36625a782ee1ab20cf47c03253250006d51868fa` |
| data_k.cpk | 210 | 22 | 2 | 0 | 9 | `4286099c3d7aa49207a36cb4a8367f493f54e0f621e1d898dc258162069eb87c` |

## Per-immediate-parent archive counts/hashes

| Archive path | Spans | Selected-set SHA-256 |
|---|---:|---|
| `data.cpk` | 537 | `a910a0333d7ef6c44fdd9faee3598cccc50863c73788b1f645b95914c004c8d0` |
| `data.cpk::battle/battle_pack.bin::btleff.bin` | 16 | `4028e247574d9fa807f3425e89389f02a69839f7e30bb9efbfdf86867afd4d83` |
| `data.cpk::battle/battle_pack2.bin::btleff.bin` | 16 | `a4ee131ffd63d95f8b70a956a0779bb0f9d6969b1056377244acac92491f47b6` |
| `data.cpk::battle/boss/e200.bin` | 5 | `28b7f15683829b71b9ea81fcb81a9392ddba3bb707e8a04a32831d5477af6432` |
| `data.cpk::battle/boss/e201.bin` | 3 | `61b7f386f6001493b0a07e9a4ce76c0dceaef20e4b1d1ad3f78eb92c43c569fd` |
| `data.cpk::battle/boss/e202.bin` | 2 | `3704d914973793692310513a772866e08eb65a6ff0d45a9ba91c36de467d1926` |
| `data.cpk::battle/boss/e203.bin` | 5 | `42be377cb1205f359203af8854be572540b384d4ce519aa72077c75a8f45e4e2` |
| `data.cpk::battle/boss/e204.bin` | 3 | `b4e63a5c6ac2fc2d9532c8b24bcdfe3aa1454d3fd20d3125d20dca25e07ed291` |
| `data.cpk::battle/boss/e205.bin` | 5 | `5367e9aa403ea8ef67ad972827ec9be2feaade9e617277ba900e3a5694dd07d0` |
| `data.cpk::battle/boss/e206.bin` | 6 | `53409853d59307002f16bdeaee5b1b0abb9c54950380f5218e528c33ea9cb105` |
| `data.cpk::battle/boss/e207.bin` | 2 | `fb24c367883b74da492f2b20b7319eacdcf4dbccf4600a494cf73daf5ee81932` |
| `data.cpk::battle/boss/e208.bin` | 5 | `6b5409bb01351d83d824e2f6b5cf8c8f03dd3f24eb0ad621965aeaadbcb56ab5` |
| `data.cpk::battle/boss/e209.bin` | 5 | `c06d62e149be6bdfb4adeb85a22440b185990eb42c21899583d0924eb81f5f1a` |
| `data.cpk::battle/boss/e20A.bin` | 3 | `58442c6e45e6e1f2405df6a7bedd83fce70240dbd03cb4997d02da18fb28cd57` |
| `data.cpk::battle/boss/e20B.bin` | 6 | `298b63763414a601b633a640b7e7e8410d6059ae09ffcd5893babbd1fd079ada` |
| `data.cpk::battle/boss/e20C.bin` | 2 | `378807de52742009018a2820cb46396b39178e0ba99ace8ad6eaa0d95f19d270` |
| `data.cpk::battle/boss/e20D.bin` | 4 | `7655b160e2a8b92a97659531c38b4d0ab3d3ae25a24b186add043ac5a9be1181` |
| `data.cpk::battle/boss/e214.bin` | 2 | `36525d975973e2fbfd03b1b0b41bdef4fc883741429db8c912bca3afedcb5173` |
| `data.cpk::battle/boss/e215.bin` | 14 | `3ad3beb8e2f599842970b4586475ffaa5b2fe85ba28fbf44c3ed97168d58791d` |
| `data.cpk::battle/boss/e216.bin` | 2 | `59f5998defdd1b59911412559b2955e3fd2a54ddc78cf6f81f3a0d0acdce53a3` |
| `data.cpk::battle/boss/e217.bin` | 4 | `3d35dd594834b8297bed53081cf2043cd4bce11d77b738d39ab627d288f40637` |
| `data.cpk::battle/btleff.bin` | 16 | `dafd73935704a507d88910c6de2e378d47a5e825afef2cea8517b45cd97812ee` |
| `data.cpk::event/fishing.bin` | 3 | `58f0eaa7cae8c9387d7eda7e411047c485c33f821ff00512f5f8cd64c4c55af6` |
| `data.cpk::event_data/community.bin` | 4 | `50a84e6e1adae8093ffa5fdd6b4fc50b0257d8cf0b85bd1a2ffafd09170ae42b` |
| `data.cpk::init.bin::event/icon.bin` | 3 | `5c2fa9887f477f801d20c23e01af9e91f02a267aa417dd9f68b6ab94c5548333` |
| `data.cpk::skill/00C.pac` | 1 | `612180b76e15ca10bee56b486292c4da9bf7cd9db18e4741d7c055d5d03c8c80` |
| `data.cpk::skill/015.pac` | 1 | `937c44db41c29cb9a3cac17a1c7029e28f68a84670374619258910f9d5c91194` |
| `data.cpk::skill/019.pac` | 5 | `614db237d436f7e60773b1fe7d891f1391ad8e6e5154c2379761aaecf666d58d` |
| `data.cpk::skill/01A.pac` | 3 | `7da1aae6a7c14245c914e93d9722f56dc1e6391a8897db637dbb5cf25e5ece1f` |
| `data.cpk::skill/01B.pac` | 2 | `54061e29560cacccb2bb6c65bc2f653b6e20244dbcb09babe6ac3ddcd7d04f8c` |
| `data.cpk::skill/01C.pac` | 5 | `691e3110407f6c917eb58a0ddc2f81bc1fc0f682559782a638635ac9e0d770df` |
| `data.cpk::skill/01D.pac` | 3 | `759c291caf44ace07285b3c6b5dc300598967daaa78850bb61a1b4c222e884f5` |
| `data.cpk::skill/01E.pac` | 2 | `1340dca798c279df8d60b9e8744902031b3cab375c0fdde9dde2999a90e92f07` |
| `data.cpk::skill/01F.pac` | 5 | `689447eff92e0db7d885d41fea60694c2a9d9170ab833917965a6ad1d0065340` |
| `data.cpk::skill/020.pac` | 1 | `319ecfca1ac7c0b6f3fb98ac9184cb4f2e5a95f5404d0f905f05c2d39d0ac2c9` |
| `data.cpk::skill/021.pac` | 1 | `4b9ba6b35a20adcaf5dddce9b236158d8f11a3ca630a9550958fb450be55844b` |
| `data.cpk::skill/023.pac` | 8 | `889d7a8498e59f9225eb0d537660f9f845024b3c35cb6ded515c0028d211e2a8` |
| `data.cpk::skill/02E.pac` | 4 | `fba3aa4bc0bc0495e4c1aea4010ad880fa18bc7e5ba5a3ece8bdd1bcfcfbcecb` |
| `data.cpk::skill/035.pac` | 1 | `7ecd0de88ed784f910d4bcbbc5074e486c4b9289e1115df9434172a65f49f1c6` |
| `data.cpk::skill/037.pac` | 4 | `d49c4f9ad9b0dced741cbf4a14151a7171fac1b008c800791b2caf997add5879` |
| `data.cpk::skill/038.pac` | 5 | `63748b4daeb583301c20e466d5c8e2979700d0ad626590354c857741a24dc3c8` |
| `data.cpk::skill/039.pac` | 8 | `b378a728b6e5108813cc1c1cc10c2ecd815147f863976b6bf65c35268e175b97` |
| `data.cpk::skill/03A.pac` | 8 | `fa24559195045c555895bef5d5e8c64c3e8eb060e4452129155f37bcefe7323a` |
| `data.cpk::skill/03B.pac` | 8 | `3e7ea70aabe7083b9b29066fc8482e3d58e09fb50deb7dc3fcd3f05983497f9a` |
| `data.cpk::skill/03D.pac` | 7 | `c3498bc750fd109e5ff4fda01baaeae0b45abe02e6a08d73fa26d97f67779a79` |
| `data.cpk::skill/03E.pac` | 7 | `2b103b918ee92f227f99ce132b43340b77c1489f0b995340ce53c910cae75bac` |
| `data.cpk::skill/03F.pac` | 4 | `cc181271e77f7df0ab524cb334ccc6c3d966de4cb40470d6a2d8b01303f32caa` |
| `data.cpk::skill/040.pac` | 8 | `58139f52c4f56578f9bb034e892533d4967fcddf662b9616e31ee6b6e7576997` |
| `data.cpk::skill/042.pac` | 4 | `599980f72b74780dd0cf469c91cf5324b5effec4760937a163990524da989dd0` |
| `data.cpk::skill/043.pac` | 4 | `33dd736c6f8e7ee746f3069a3a3558251641f1de85257e0c6fbea8d923ba0688` |
| `data.cpk::skill/048.pac` | 3 | `df15316cae38eeca4788316d433b4cfd25cd7a20ac147e1dd34eae290950b211` |
| `data.cpk::skill/049.pac` | 4 | `a41c8c0443e5617a19dd0a01eded39df8c0973c92dc9d3c0e753b594672fed12` |
| `data.cpk::skill/04A.pac` | 3 | `68039c12ffa37af9bc2a6fe2f9f0e8546a8a8182aed074054238fb09b7e6e5bd` |
| `data.cpk::skill/04B.pac` | 10 | `cb60df81f20b211bb477aaa973865162ba06aec0cdfb25b852ba79d6ad5fa32b` |
| `data.cpk::skill/04D.pac` | 4 | `30e46b7054ce1a8d7387ac8db80d49649cac363d1857cbed6c4a6e42e77c852e` |
| `data.cpk::skill/04E.pac` | 9 | `5149ac6a0ff65de122db0109b1c11554ab9feba823cf6638d3c407f8b58954ad` |
| `data.cpk::skill/05C.pac` | 2 | `9cb0625aa429dd4746fc4c028194713ac3689b89885ee321fadecede37fcf368` |
| `data.cpk::skill/05D.pac` | 2 | `76601445b7a526214364d08a83a4b6195cbdebddfad8daf5cd7c13b20d55c15e` |
| `data.cpk::skill/060.pac` | 1 | `dbace325ea01be80485bc806343b789406727f2fcc50d0d75cb2caa8f55d1f93` |
| `data.cpk::skill/069.pac` | 1 | `e4a27e41cbdfadc19458eab96f53278134bbc221371ab1943e8ecd78f6c6057a` |
| `data.cpk::skill/06D.pac` | 6 | `e37ffce7b2446ff8e583d6dac4404ad6f5ae7dfa7aede5ec7ed45cf01c1dafb6` |
| `data.cpk::skill/06E.pac` | 6 | `ed0c90a34c040b0b65dd9ccc9d47409ca68ed302174916aeb4b36c0093e5ed01` |
| `data.cpk::skill/06F.pac` | 6 | `06dd5095f5a5c3a4c0dfd8c2ab929cae4954298650c4864d4556f456c5e086f0` |
| `data.cpk::skill/070.pac` | 6 | `8e8e3a5d9917257a915070ede929e8ae72e9b4e205c698f1ee3b40b4e965a271` |
| `data.cpk::skill/071.pac` | 6 | `eb1a6ac452845cf44df872224a2168f354adc84df95552ae85522323f27ac7f3` |
| `data.cpk::skill/072.pac` | 6 | `ccbf1f9d7d041667fb295d81aa4158b703727ed334e6c1d543bfe694660dfc8c` |
| `data.cpk::skill/073.pac` | 6 | `bf894d0200a5956297503a0e90bf378e006ad399a4cba60cddcde1edb312df22` |
| `data.cpk::skill/074.pac` | 6 | `19209be3d7d1f6c3865d2b0de91db421d1bcfe4d12c222e38cd632039a48b7eb` |
| `data.cpk::skill/075.pac` | 6 | `8a7ecefdf4fb085b0c84c77b412fc85d75b8742ec4d302ce813dc00c00880421` |
| `data.cpk::skill/079.pac` | 1 | `edcc62b411849a59c21fb8781dee3436b3b9f16006e9a8fc85a037479612c493` |
| `data.cpk::skill/07E.pac` | 1 | `6fc04a9aae8e1b842db252fb43c39c396e6314a3b819d765fc4dc50597fb4bc3` |
| `data.cpk::skill/080.pac` | 1 | `4f054085e932dabc4a629d2fbd56b87d70b8bf920c051b9ae55dfbc13bb5d8ff` |
| `data.cpk::skill/081.pac` | 1 | `f519ab23a8c371d68b9b247c76110a7d3e0e06ce8d5e44eef76d78c2fe682876` |
| `data.cpk::skill/085.pac` | 1 | `3c1e405a777fa6f32ba9a0183c4565bf11362a7ef7bee3cbe8e78d15f4df4425` |
| `data.cpk::skill/087.pac` | 1 | `7a600fd656670fce1cbee4aa1efdf19d468531f84eedfd7562e03398b2a50ca8` |
| `data.cpk::skill/088.pac` | 1 | `3a8af78838e38551b05b28f7e866d938418ba5b439d535e5bab7f48976b65d3e` |
| `data.cpk::skill/08A.pac` | 2 | `5bf32a46c6524614847fd2f2dcbc81fd2f046b12f36524d50c41490573feaa83` |
| `data.cpk::skill/08B.pac` | 1 | `6c32f5bb3c692075e0d3c0f6c3a9a0a123757c76f6e7a68e01aef8450d04d419` |
| `data.cpk::skill/08C.pac` | 1 | `579418402cf78631381a5053c119275a3bbe81c68223d3233fa0745e7d457dd3` |
| `data.cpk::skill/08D.pac` | 1 | `c89fb052cdb36ed2c9fe4f401f84d7d6fe5c9eed02329d41e67e9127a3280f0f` |
| `data.cpk::skill/090.pac` | 1 | `949f1d83ea37235aba7b240dcda289be37b0673372943962bdfd635f0666ea18` |
| `data.cpk::skill/091.pac` | 1 | `1eb1ffe3b0522b8be5cd1e12de443c70cc4904bf1e9db5119c6ed2f5239438ac` |
| `data.cpk::skill/092.pac` | 1 | `04ee3f230426669e82e8d17ec0ba070da4f2ae02eaf2d1b1cf18d9fac64a2d82` |
| `data.cpk::skill/096.pac` | 3 | `ebe7a490c6c88f4d6034753ad5fc4613c8de50fac27645dc197da85246b592fc` |
| `data.cpk::skill/097.pac` | 3 | `d13da4fb849b932f66c5ad56852051b57ffbc99618b219a4f01c5bab97f3850f` |
| `data.cpk::skill/099.pac` | 1 | `891bb01035c37dabed40231b3ec87375aeb421eddc13f8e2b7e876adc5d175c3` |
| `data.cpk::skill/09A.pac` | 1 | `c1a44e2d9f0545d81635fa77217e71f7068f99a9854cf0c12af439c5ac067288` |
| `data.cpk::skill/09C.pac` | 2 | `9dcab0a14d8b925ddb6d632f66e5505ba942725c37e512aae91a995746b463e2` |
| `data.cpk::skill/09E.pac` | 1 | `52a1668c40b0f38f027ef33c6c60f8fe6a9a30153c539b1a33b349b06667a2ae` |
| `data.cpk::skill/0A0.pac` | 1 | `3152c752ac6211bc8acd5bd04135fe9353f0d9f69e6643c8416dc48fdc6fcf49` |
| `data.cpk::skill/0A1.pac` | 1 | `830c66d53f8eac8451d95708d71531fdb94e86eff3d50bf73b525671cdc2a6d7` |
| `data.cpk::skill/0A4.pac` | 1 | `35cb9d5f11e880c9e075d34a822c73949f2b25779aab3af5176acfd36936ba8a` |
| `data.cpk::skill/0AA.pac` | 1 | `1cd962d8f767486448f83b4dffb132d652b5a60d362c3b08717b921759df0678` |
| `data.cpk::skill/0AB.pac` | 1 | `f51e9e7aeaa4ae7fa1cd7b17785eceb29ce3a0833bad8eb3d5589e536b447d17` |
| `data.cpk::skill/0AC.pac` | 1 | `3d387db32896f025ae8d390bbf842cd059f6dfe1360e5f5de5a8c2623e827ef8` |
| `data.cpk::skill/0AD.pac` | 2 | `90bcc71e82e28ccc0e35ac9d04ecf77e78b397db0a35acffdaf05bf36c6b250a` |
| `data.cpk::skill/0B0.pac` | 1 | `932a0b34d3fefac6b5e6075c975d3e65df0ff5d29fc2a6ca9d11429cb720d808` |
| `data.cpk::skill/0D1.pac` | 3 | `b465b14ea41418f1941fccf82e3c3ecdcfafb7092564ee062f2a2c15bbab2ea1` |
| `data.cpk::skill/0D2.pac` | 2 | `f1e9b65839a35f72d6f6534b8e6b714956584742fc9ee2d98e28400f44523d65` |
| `data.cpk::skill/0D3.pac` | 1 | `6b6eef3fb83e7601657d3c33348e6bba3ff1b8bcc754e7186985a426fda39e94` |
| `data.cpk::skill/0D4.pac` | 1 | `35bbdd57410ea18fd6d4b8ae995fdc69ea2440988ad2822f4fabf6748c1c43a8` |
| `data.cpk::skill/0D5.pac` | 1 | `b118d7ebefbf41ac5ee8e9733815183201a9d282e12fecfc5003bc6224c04c12` |
| `data.cpk::skill/0D6.pac` | 1 | `a8bd4675db63b69dab969591060a164930e360b7eb8207b1655ab2595e324dd0` |
| `data.cpk::skill/0D7.pac` | 1 | `deb97c72febd7fc601d1447df3926d0778c88e1eb33405e67c5a30848157b307` |
| `data.cpk::skill/0D8.pac` | 1 | `07382ecbb904e6e1b45ded0223ba07ad22ed2f46740bf6f3767ca93fcf4f3857` |
| `data.cpk::skill/0D9.pac` | 3 | `e71c49f6418b10984c3aa376a6425893d6e4c8551d6cbd4a37eaa2bcda0ee3f4` |
| `data.cpk::skill/0E0.pac` | 3 | `d576f687833276c84e32314ca9494b71cf8578cc2dd565cb223e80b996e27b46` |
| `data.cpk::skill/0E2.pac` | 3 | `55c59c41649e7509122d2ba64fa76e8d41b8196ddb767c5cb0239f9134334789` |
| `data.cpk::skill/0EB.pac` | 4 | `8a8b271f904b81fedf284a79715d7f02070a73e2ac4d34dc7ac4919dfd8ccec4` |
| `data.cpk::skill/0F0.pac` | 8 | `52eaa30b0716fd8466476ca8a7f3aa6dbf8212d227494bf2a7880d1f3eb78a36` |
| `data.cpk::skill/0F1.pac` | 8 | `b0f5447b0475b1154067c15ade26dc2080445cf6255aa501667f17ad5f534b5e` |
| `data.cpk::skill/0F2.pac` | 8 | `5d2c514259c6165cd43d8024eae5c1cbb0eb05c80dc6705b82a2659ae882b0c7` |
| `data.cpk::skill/0F3.pac` | 8 | `12f24a697fc24213302d1765d1d8fca4a9fe380fcb1fdd20fd87245fa8d1b8e5` |
| `data.cpk::skill/0F5.pac` | 2 | `a72e4aec9cbe8db62fd6f1b9436861d0495f60e2cda75a398d4cf949dabda27c` |
| `data.cpk::skill/0F7.pac` | 3 | `7f2954f8db5d830d94a374b64d39ff97cdff063af4feee052a6290cc4e952ca5` |
| `data.cpk::skill/0F8.pac` | 1 | `948de6dbc9b86583e118d48ec9452d896024069dfbf037e0a30d0c579702c51e` |
| `data.cpk::skill/0F9.pac` | 1 | `f7f9ad0360510cd18012075c05609c806870bbca61a337cded8fc732cb6dfe1d` |
| `data.cpk::skill/0FA.pac` | 15 | `fa6fd65eb53a890960edd4c65325e5f691105f9d237959459314415b4ea154a0` |
| `data.cpk::skill/0FB.pac` | 5 | `6593c896bc72fe38e5a90f6d51116af835b61589e366457020d228787cb4b070` |
| `data.cpk::skill/0FC.pac` | 1 | `3d18ddf0cbd6d4e7da7d9e77744c3a1c29380e9ae8408bc078b49039984a94ac` |
| `data.cpk::skill/100.pac` | 8 | `17dafb423cad8c0e91f5e3f42bb24b0a85766261a4347bc1f169d433e1431684` |
| `data.cpk::skill/101.pac` | 12 | `379f25430fac3ad383d045a9d6df852bca7fe8e80fe4c0723807002d0f0744bc` |
| `data.cpk::skill/102.pac` | 9 | `29fe1eeceb0f91f9a7ab1011ca5175cffa37ae793da2a50039736c02e4d44e95` |
| `data.cpk::skill/103.pac` | 5 | `42dabd3474055a6eb7df37d247b2f0cdaaa610462998a42a16a26bb4ec26886c` |
| `data.cpk::skill/105.pac` | 2 | `fc6d51f6b50b004fc80c8c591fab8c81c17426a9849c57fb3d3f016a618bc51a` |
| `data.cpk::skill/107.pac` | 2 | `7ea22c1bf51fe1e53cd17880f34d91f456d993f9ca45cba7b72ce3818c7dbff7` |
| `data.cpk::skill/116.pac` | 3 | `e721e9880b4797ac447c295da0329b427eeff7f705d3ed5173fc03b18b18ae6c` |
| `data.cpk::skill/117.pac` | 4 | `8466df1b6581114209bba9d3906e5c2d9cfaf0bcd068a51487f25ecc17f7c4dd` |
| `data.cpk::skill/118.pac` | 2 | `7fafea8b3d1757567e97e702ec9b375701faf17c87024415f10a4584fc64ef25` |
| `data.cpk::skill/119.pac` | 2 | `49dc521458cf9698b02dff8e554a75a689dae7c1544e0f914b9492d10dc09b2a` |
| `data.cpk::skill/11A.pac` | 2 | `e92e2137be9b197742b4598c582f43ef63256626cb9a422013c7c582bcfee086` |
| `data.cpk::skill/11B.pac` | 2 | `aaff01187b2142177c67d65ab80599cd20cfc846e3be29cb22e35b29b904accd` |
| `data.cpk::skill/11C.pac` | 14 | `7970becc18f1ca8d7ebca28f86147b8ec2a44879f410d7b9bfcbe19050f37919` |
| `data.cpk::skill/11D.pac` | 2 | `5a2d4e5b025b32a1754e9c6b1a735a56c29cee729292377a4ecfca49cc598c8d` |
| `data.cpk::skill/12E.pac` | 3 | `60d3262fb8cb85b9ada5371b89ec16fa71e4b09aef71aa37229106b654297804` |
| `data.cpk::skill/12F.pac` | 2 | `b72deed104389ce6e7bea0ff8045db4c5c094df55e54aff239c053ff6114fbf8` |
| `data.cpk::skill/13D.pac` | 2 | `a09fec815a20d55110985c013934966333f11a39edf789daf967fd8031e78288` |
| `data.cpk::skill/143.pac` | 3 | `58f1ac0a1757ad8a8da38fe5b74938ae67b6c0038014d17a5a2e4141852f82cc` |
| `data.cpk::skill/144.pac` | 3 | `8bb2e785f1fe10eb37e571c6cc5e11b86dcde6faf2966fc0ecdb1af19c64188b` |
| `data.cpk::skill/147.pac` | 2 | `dd8b8a0c7f78555f862fa14bbe7374e4fb498d4aa8abac6545928f6139419698` |
| `data.cpk::skill/148.pac` | 3 | `6e9c19195dc17d9100650f3cff4bcbf1c56fd28516c1d7a4121525a0726d8ca3` |
| `data.cpk::skill/14E.pac` | 7 | `e9f8f8c30c944edfcfa6ae78d6c503565b4dfc18f6746f0e07c1ad961f267078` |
| `data.cpk::skill/14F.pac` | 4 | `4fd5078819be04cd20184b1fa5d97134f5c60e02fd81ae87ef503e13fadffacc` |
| `data.cpk::skill/151.pac` | 1 | `3ddd056dc0a39630fc01cced9348488173d6aea0c3a629c9294a6c97ba3e00f3` |
| `data.cpk::skill/152.pac` | 1 | `a80cf06e49382c7f4b0c0a89cf261762404e60b220daec87bb5c3ce420eacbf8` |
| `data.cpk::skill/153.pac` | 4 | `39fd87f55621c9394c62bcdd32e9c0a12c50da080022a35fffa7b10bef204986` |
| `data.cpk::skill/15D.pac` | 5 | `c4aa34ea559fd4238bb6cba2f34472cc59619905b20aa225a09c90b2fd61999b` |
| `data.cpk::skill/15E.pac` | 6 | `78e8abe8581c60f1d60d9252a6ae3a6939c5efc9f34be7df1b7f0a6d291e56f2` |
| `data.cpk::skill/15F.pac` | 3 | `cf6202837d49dfcc3cb9da572ca58baf6a5572a57e243b34c8b847b38584057b` |
| `data.cpk::skill/160.pac` | 2 | `b86630c5c6a75699aec356398a15f29fb2bd81a4d8315524616da2caf4071847` |
| `data.cpk::skill/164.pac` | 2 | `ecdf83c5bd3cec76e66f1e94cdf404dda9a63491f90a17da98348c37df32819f` |
| `data.cpk::skill/165.pac` | 2 | `300f5f139d3797772d7f4025692858f66dd2f4e730fc640ff291143617b04a03` |
| `data.cpk::skill/169.pac` | 5 | `b8efc709d013c54c39d0d8f68603bc7a47bd3e107223c73cab35281088e7619e` |
| `data.cpk::skill/16B.pac` | 1 | `46016e798fad4afacca6e48c5b5fc760a75c2d011bc49418a39dc2e67226f32e` |
| `data.cpk::skill/16D.pac` | 1 | `313f78bd637b677d0a59f2486585a0c2df1738c50fc7531fdefd251f348627eb` |
| `data.cpk::skill/16E.pac` | 2 | `2f6fe1faa21abc0d830972d7add9347279fa74d9d45961d11a9f1be4972b8609` |
| `data.cpk::skill/16F.pac` | 2 | `fdddc00cbd046db28e0ad7037e8f0f9a3bc7420abcfa9f8be382cbadc1aecd05` |
| `data.cpk::skill/170.pac` | 2 | `9083015feeb50d415c22b94789f82080b307eb378e7a8cb8b8dcde8a4e46603d` |
| `data.cpk::skill/171.pac` | 2 | `c53ae02cf505e1ff94cea863468f7874be5d382950c7ddf44f7fcb565b3ebdf4` |
| `data.cpk::skill/172.pac` | 2 | `46dec3f4366853cf8abd70c62955bcf97d82166298a5ed68c242c627425e60d4` |
| `data.cpk::skill/173.pac` | 1 | `b437eaa8f79c57b0ec29552529dd9d1b3f75a580b993b5035756b628f27b034b` |
| `data.cpk::skill/174.pac` | 1 | `a293142e95cec543b5be721452e42c5c1918355bba001e7fb063348d74ed627b` |
| `data.cpk::skill/175.pac` | 1 | `421fc37961cf925d220f22b34379693d4fb4c986525d4a702ef44f96168b01c7` |
| `data.cpk::skill/176.pac` | 1 | `876d575ee7390bce5818ccb2ef78b7a53563c5dde9fb53af40311e88f327732d` |
| `data.cpk::skill/177.pac` | 1 | `fa9004aa80d275d3470f2ccc425d983094213a390e6fc420a7d6d4476697d5e2` |
| `data.cpk::skill/179.pac` | 2 | `5addc9b817c4a108b89623b13f377598f420c88d147f6b580a93251f63fa4869` |
| `data.cpk::skill/17C.pac` | 3 | `a2b617916dc67453133535ae8ab0ea7c831ce873986d4babc7d5dc098e4f63a7` |
| `data.cpk::skill/17F.pac` | 1 | `a467f631c9e20cd9673f5241c78985155122b4b15556dbb227435050b34c8915` |
| `data.cpk::skill/180.pac` | 1 | `d46ebbe00ce5e5c9b58a76a6e19ede62fa9311d3a1795aa0d001a9d0405affd4` |
| `data.cpk::skill/181.pac` | 6 | `478e9e958ae1028794cb019bfe1b39f6f94375a1820032b741b297d5559810ef` |
| `data.cpk::skill/182.pac` | 3 | `d7db87ee1031571640ada6bdc25180748b06d798405f62b8701a683dd119b42f` |
| `data.cpk::skill/185.pac` | 1 | `96d436edbe42c1bf3427edac2ac90e9e4816c3c4006a810e44770448e58b2ac8` |
| `data.cpk::skill/186.pac` | 1 | `dce0c2030d0b40124ee97b87169f103eb0ec4a0b6bf266761afca4fda8b747ff` |
| `data.cpk::skill/187.pac` | 1 | `f159c32ca92458ac21a6284d5922af1cf30ea85277a2c0d47d2464a94593835a` |
| `data.cpk::skill/18A.pac` | 1 | `6b73a4bce383d117e3e8ae180e4b71aa7f409aaf47e6cb5359d3fa003083cc58` |
| `data.cpk::skill/18B.pac` | 5 | `e743997902a79cc446e192949a0687dce174832de48c8e74d4c7d495cb08747c` |
| `data.cpk::skill/18E.pac` | 9 | `46c1b202acb656142c014c74c318089254df7d446d06e6a36892faed3843b3fd` |
| `data.cpk::skill/18F.pac` | 12 | `516a1353fb6a12b4b635049a1d9f3b4a59283d12326353979e67dba60b664955` |
| `data.cpk::skill/1B9.pac` | 3 | `b98885f46b85bc8030e077bc4e5476de416a5bb841aef07d2a4a939fb46a8282` |
| `data.cpk::skill/1BE.pac` | 2 | `c624abba6c0f87dcd651e00280019276533e43abd34a59cda6499082cfbb0a8e` |
| `data.cpk::skill/1C2.pac` | 1 | `98304508fa5953256a596a6727863769e12083ffa2832b48fe93e0a53daf0752` |
| `data.cpk::skill/1C3.pac` | 1 | `ea54110497cf66054e952e2552ea7c2e3c2b00e6824276460e8ff87608c01381` |
| `data.cpk::skill/1C4.pac` | 4 | `9075568b32a7ea76b2b17ef8293102644132bc422b0f9447b4d4501e908691fa` |
| `data_c.cpk` | 33 | `6f4c142d0d61ea928d6ed682724b8a5da9c7ec361abc894cb90558dd9918f627` |
| `data_c.cpk::battle/battle_pack.bin::btleff.bin` | 16 | `de1b103e4bbc15256c1eeacf7b3a418685c4d092b55c6b6c5307f31bad93cfcb` |
| `data_c.cpk::battle/battle_pack2.bin::btleff.bin` | 16 | `2b41336ccec17f16f2bdcee3a3a53977e5848d6fe6067468ef26a75cf6019b61` |
| `data_c.cpk::battle/boss/e200.bin` | 5 | `dfa8f1fe975b8a53ca60e9acb629bec46f0ec7ca689343148951ac02b292a8a6` |
| `data_c.cpk::battle/boss/e201.bin` | 3 | `b50c568e3813f2de16be69e8aa0eefba423892ff023bd23b785cae4e910d28e1` |
| `data_c.cpk::battle/boss/e202.bin` | 2 | `100d418794d1b36b9c1c914ae5506672760b97fe17e7e0d9bfe100d1ac1e3eec` |
| `data_c.cpk::battle/boss/e203.bin` | 5 | `cd3646ea546e861a082b4bda0601a91d5bcb28e5572b6e90bb3bd67845a2cf6c` |
| `data_c.cpk::battle/boss/e204.bin` | 3 | `57cf4b7708a555b7df78cf92cb893b6c4a7be7138ec78a0b535b5b3c5cf5739a` |
| `data_c.cpk::battle/boss/e205.bin` | 5 | `6b43b2a6676590daa61b69668a644e0173b543152bcdec1ab48fe2b1da7bf0ae` |
| `data_c.cpk::battle/boss/e206.bin` | 6 | `4c50291c4b30214d353387e72fa11a11e53c1837c65448222c8e8562d3d31baf` |
| `data_c.cpk::battle/boss/e207.bin` | 2 | `6ba7e379407d91afeca5f4b7c0b327878995ef13700f805b4c35b69b80c47d20` |
| `data_c.cpk::battle/boss/e208.bin` | 5 | `3f1bf011224624297d4c3eacc914ee33f14e6f6c9495e33fd015f3b57c13ca71` |
| `data_c.cpk::battle/boss/e209.bin` | 5 | `1eafe6fcbf305b152980f3a54e1b9af2d3296fb60a58263151cf7bacc15fbb89` |
| `data_c.cpk::battle/boss/e20A.bin` | 3 | `a2397024b2a49e500faa945db93959ce1cf80b1e84369c8442ff96495b7afc7b` |
| `data_c.cpk::battle/boss/e20B.bin` | 6 | `cb594a514b8550bd94a91e569e65ad9e1cc42d2bc39132ff82bdeb48cc86a88a` |
| `data_c.cpk::battle/boss/e20C.bin` | 2 | `0c7277795d8cf570f34c6d085d03311ed04f4c1a6e76efc85379219c7ba6002b` |
| `data_c.cpk::battle/boss/e20D.bin` | 4 | `d63a403effdc0f6a1bb619a92ef9860ace9e39e7fd6059094105b4e76fa9e7da` |
| `data_c.cpk::battle/boss/e214.bin` | 2 | `d1033ff3f508683c4a416d5934263ae2927fc5bd7ce723aee0618897deb34962` |
| `data_c.cpk::battle/boss/e215.bin` | 14 | `e52c5351d9039668d67bd9f4645bc04d8c7efea8ea42e64f341fdd25b765d9f5` |
| `data_c.cpk::battle/boss/e216.bin` | 2 | `cd2c220b578fe0daf3ac1cfbc3d7cdfb4081280da26eddea7f5b7899bb4026ba` |
| `data_c.cpk::battle/boss/e217.bin` | 4 | `1419ed689572b15be7037ea438a036af09164db3af35ab908ac8245417e3dbed` |
| `data_c.cpk::event/fishing.bin` | 3 | `9b7f8a1b6da2cf0137ff8d7a1e20dc3ef6607798b637e65813efe0ce06477563` |
| `data_c.cpk::event_data/community.bin` | 4 | `b886fbd953826183d760977bf95531691a8ce44bda63e2b07be29a7ff2000cea` |
| `data_c.cpk::init.bin::event/icon.bin` | 3 | `29264eaeeec9d3aaa6d2321de099b981717c8e0bf9eed0b5d2fe80e6ba42c91b` |
| `data_c.cpk::skill/100.pac` | 8 | `ad3741aeddcc9a0222ed09d9bc11d297c71badac44349a60e8fea830dd955d7d` |
| `data_c.cpk::skill/101.pac` | 12 | `a3ab0e86a3142bd840f3dbbffcf16a87a937d9988f6bd47c30c78741780d15de` |
| `data_c.cpk::skill/102.pac` | 9 | `8a23e5e6deecd58c2b56b21ba7af91b33e23ab38cc89656d89b827295b8470ea` |
| `data_c.cpk::skill/11C.pac` | 14 | `2381aa872d56c43f076b7ad70a38bcb56656561b9a07e2fd8a07e55e9171a8ff` |
| `data_c.cpk::skill/172.pac` | 2 | `43a7a3a48736bed19ac792ac7997a86578d4228ee486d22eaffa25274ed6a4df` |
| `data_c.cpk::skill/173.pac` | 1 | `9c2c574985802e9985ba2396f9939f8666be66770c8bb7714fbe83cb4a57469e` |
| `data_c.cpk::skill/174.pac` | 1 | `57cbeb58d4db04ff2cf17c64c8dea8fe4533fa9b10cbadf8783ac1effe1110d6` |
| `data_c.cpk::skill/175.pac` | 1 | `f10b2fa13c9d3d7ba5b8dea498fb3e7cebbfefbef42bfce7091754f5db038ae3` |
| `data_c.cpk::skill/176.pac` | 1 | `222d5f541737284943613e7cc5f6eca0831ac8a617f9a4041797d21b03f3a8f3` |
| `data_c.cpk::skill/177.pac` | 1 | `8369114b2627729157bfc18bb90a29b023b542b986c6efb8ce3e70dd89e733e0` |
| `data_c.cpk::skill/18E.pac` | 9 | `526c99876487c20e0bd0fb551b948854280328ff04b2ad2f8e0b185ae0fe7455` |
| `data_c.cpk::skill/18F.pac` | 12 | `500aa5c8765b830a00c959af12b3f1d19d81a0e54ee27230531f711da7139df1` |
| `data_ck.cpk::battle/battle_pack.bin::btleff.bin` | 16 | `5846d0552945a1ec5e4b7688f365ace47568eb8dba70a36254ccde523f6b17e1` |
| `data_ck.cpk::battle/battle_pack2.bin::btleff.bin` | 16 | `77941b9c0b0ebf98c104aaa4f4dfb47ffa136926504b217c7e2b841801172177` |
| `data_ck.cpk::battle/boss/e200.bin` | 5 | `3cd06dd80563f8e51bd40c0bbd77727358337bfb8d9dea7384e45064ddfe381f` |
| `data_ck.cpk::battle/boss/e201.bin` | 3 | `a2e707d09dce4b8b7258adc036a55998300879e6131521f38b9136fa2c019703` |
| `data_ck.cpk::battle/boss/e202.bin` | 2 | `6920ebfbaf19b9e889a36872e05f86a2c141745b617ef5b14b954c7f09a515a5` |
| `data_ck.cpk::battle/boss/e203.bin` | 5 | `8c9840762ea48b2ef0088056d06c7a6bf11a292880d6d6c7d406007bb225b434` |
| `data_ck.cpk::battle/boss/e204.bin` | 3 | `b23a9a300f9aab79862e6bd59ce5f3f9326f584641d3bb6af42367ad9c4c1f43` |
| `data_ck.cpk::battle/boss/e205.bin` | 5 | `90253775bbfc693686f5851ba981e2557ea974f0efcc983d515bd0e40ecb347d` |
| `data_ck.cpk::battle/boss/e206.bin` | 6 | `816b89f170f3276677c5452b72ed43e4b75704a2b8aa79652330f36577655820` |
| `data_ck.cpk::battle/boss/e207.bin` | 2 | `6c958c8a9dcb7f7b322a26b0f006225e1973ea9d28abb8e2f92d5c453b3d6783` |
| `data_ck.cpk::battle/boss/e208.bin` | 5 | `fe8e7ee3591dc137641e3d54241a5f71aed16d979c655f0b4d4f32e4e4624c0f` |
| `data_ck.cpk::battle/boss/e209.bin` | 5 | `de660253cb51c9428dcff19c8a545ae8a5c65b30cc45800c7529715334c13e89` |
| `data_ck.cpk::battle/boss/e20A.bin` | 3 | `0a5ba0ecc3ecfc708e36ea70e7753eee57c06b08435547d4cfcac06ba452f5a4` |
| `data_ck.cpk::battle/boss/e20B.bin` | 6 | `5e9d153d811630b3c83498eeeb12dcf51d14b10f6dc81dc50f7214f2f89b097c` |
| `data_ck.cpk::battle/boss/e20C.bin` | 2 | `3a8e9a3c689516f188f671758f48ef765e31d5f330248121f4f457ec453c2023` |
| `data_ck.cpk::battle/boss/e20D.bin` | 4 | `6312ab1bb2938e4025fb3178d645fc559c64d53c4c6a59363e5cd30a0060331c` |
| `data_ck.cpk::battle/boss/e214.bin` | 2 | `966213ee59bf5b72fcbddcb68ab9bfbbee98a3d33750195ed20423bb09c1f811` |
| `data_ck.cpk::battle/boss/e215.bin` | 14 | `b9b423b1629f8ec6bb99ecf4dfdb9d9125b115f363b3acd5d6848993a5d5aefd` |
| `data_ck.cpk::battle/boss/e216.bin` | 2 | `5f67a22b4e51fd00bbb604b72ff995cd0fb200f598e1ed8364bd9045726ab3a1` |
| `data_ck.cpk::battle/boss/e217.bin` | 4 | `008680daff0ae4900ddf247d3c3ad1b3bef053783b2b4ab1ac42a590c1fc1e7a` |
| `data_ck.cpk::event/fishing.bin` | 3 | `855b24144ad6a1efd43148911f7118e95319c265d55caeabedc0c8cfb712587e` |
| `data_ck.cpk::event_data/community.bin` | 4 | `95d152ad31bf69fb2800d43fc78033e1bc5242218182e9775eb71c8ae5608627` |
| `data_ck.cpk::init.bin::event/icon.bin` | 3 | `3474a84451cc7662f6b9fd2efb13733276e43a2d29d5ffc409e78c2f536bb9bc` |
| `data_de.cpk::battle/battle_pack.bin::btleff.bin` | 16 | `4b616536b9f6ccf1edac266433337ad1d83d6165f52beeae1c299417162da597` |
| `data_de.cpk::battle/battle_pack2.bin::btleff.bin` | 16 | `8fd88cae199208c4292e30fb1ca8e3b7f3fbf8fa1b74c08f6f45cd4e69d6dfd3` |
| `data_de.cpk::battle/boss/e200.bin` | 5 | `150ac42805711a3d9987f23497f097270699ea9e515578660c8884876fb23b5a` |
| `data_de.cpk::battle/boss/e201.bin` | 3 | `e2a1fe0d0ff395a6441069d594003c8e181bf99de36493bfa2753fac034c5beb` |
| `data_de.cpk::battle/boss/e202.bin` | 2 | `88d8eb13c13fba313526e93f9a9d9bf5fb8e0ee5788b12e18659d4c7d5d44e4f` |
| `data_de.cpk::battle/boss/e203.bin` | 5 | `7d6ba5765b5bbdb8dfe48065ec435f727e11fb95d4c3312413003fb7fa60a0d1` |
| `data_de.cpk::battle/boss/e204.bin` | 3 | `f39f7ae2bedb0c32d4993757f3fd15fb1fd331af24b293cd329fa8615d013a42` |
| `data_de.cpk::battle/boss/e205.bin` | 5 | `0a031442613005783556aa3916df5818f964a64b5133cc54862b3408e3ebe47c` |
| `data_de.cpk::battle/boss/e206.bin` | 6 | `221493eaba76659674fa41804aecf81ebfcd4f9d848577f8bdc2e1490fec33f1` |
| `data_de.cpk::battle/boss/e207.bin` | 2 | `52aa5bc5f0061f260e69108dbffbf7ddda57e67baa27a745b2f89000c6445679` |
| `data_de.cpk::battle/boss/e208.bin` | 5 | `26dce8f8ac8b05417107dd84b4b805e59dfa35af5cb8c55ffbf980d98e09ae6a` |
| `data_de.cpk::battle/boss/e209.bin` | 5 | `18619731ca8f078dffffde7cc336858eba803c02cf5ab6eec9f0555d35f164ec` |
| `data_de.cpk::battle/boss/e20A.bin` | 3 | `4d966247f9c0089f4b4e1e45296f2381bfd4a9c1cda662f738d365605325ab18` |
| `data_de.cpk::battle/boss/e20B.bin` | 6 | `cd677bbc0f7c0c63057229f027c53a248cca03496691de9d27f20c8bf5598fae` |
| `data_de.cpk::battle/boss/e20C.bin` | 2 | `911a6606a4694dd4683a23861a9c009da3fd3084a19d52444216f9a6529a3f1e` |
| `data_de.cpk::battle/boss/e20D.bin` | 4 | `96f9e99da2080386b869bada97dacebbf4f384447f5bce25087461d1656f45c5` |
| `data_de.cpk::battle/boss/e214.bin` | 2 | `e223e82a4786263875efdfb1427eeedf144acc8743a63bf5d4edfaf7d5961a97` |
| `data_de.cpk::battle/boss/e215.bin` | 14 | `fc233108acf7a9cb69da14a15521c921561245c76298310cc3fe39c4aa82db7b` |
| `data_de.cpk::battle/boss/e216.bin` | 2 | `6f8fe372c19b47f72500b16d512a098c2890779182a0a027b2881835053ef797` |
| `data_de.cpk::battle/boss/e217.bin` | 4 | `dbde44ce00be7d7c0282c5f06483a33cffbaa4bbd48200dcd8ab2e85c0779306` |
| `data_de.cpk::event/fishing.bin` | 3 | `9d1f0147276a14746f8b51f07a63381c9997f7ba78dbfaff3322e0a7689216ea` |
| `data_de.cpk::event_data/community.bin` | 4 | `1c1756d94940a12b72ee9fad95dfb8263916fd87c6e501d9ea5b9b0a9e422d54` |
| `data_de.cpk::init.bin::event/icon.bin` | 3 | `c4cb54e13a91b688215ba1abfe55141cb528c8d129d162d99699bd1b091b0095` |
| `data_e.cpk` | 36 | `b58ae49ba2dce434b75457367a57ee0fde3886ffd9469b6540b54b51c1b3524b` |
| `data_e.cpk::battle/battle_pack.bin::btleff.bin` | 16 | `89b550b05adc4d80866de33221450c801c79d5c3b545fe033b7090fbd80a0713` |
| `data_e.cpk::battle/battle_pack2.bin::btleff.bin` | 16 | `bd5e6d229e69f3210bf24016c45653520892cf76adb3bb6f1a8b7071fe5ce9db` |
| `data_e.cpk::battle/boss/e200.bin` | 5 | `c899ddf954b4606f722603d47e70db486935d314cffc40216a88704d4eccd05a` |
| `data_e.cpk::battle/boss/e201.bin` | 3 | `fb0b5c79083f1b8ad6ffc8869831e0cc5470a839ece345eea53cbed2f809f090` |
| `data_e.cpk::battle/boss/e202.bin` | 2 | `d31c1c677526c398b08c04a281a99057c12440d19da1d101fbb7d57f78deb1d5` |
| `data_e.cpk::battle/boss/e203.bin` | 5 | `e19b6470aef0799796e016974269ee1313d8104cf2692b03487e33b880bd2e8c` |
| `data_e.cpk::battle/boss/e204.bin` | 3 | `908e362d7f7ca883ed3a86bc639b3ab7d6a50cfd2563e5943a060d4a33ab96b0` |
| `data_e.cpk::battle/boss/e205.bin` | 5 | `405094f3d7a3fa9cc6563ee1fb108fbef1d26623459d5d6c7136b3cd44a95df0` |
| `data_e.cpk::battle/boss/e206.bin` | 6 | `5ff4b032455651669da476130dbb07a42497365abb64d4d274651b141d6e3170` |
| `data_e.cpk::battle/boss/e207.bin` | 2 | `d9f9bba4914d04862e406ebb6e18ae3cdabc8e9efaa62941d4e9c8b291676b1c` |
| `data_e.cpk::battle/boss/e208.bin` | 5 | `f1447c2274156e3aef5e7eb654221f70bb87ecc809c7eefc442664be88348c7f` |
| `data_e.cpk::battle/boss/e209.bin` | 5 | `1acecfb3415a1ed6041df60c8bb650c63c7a616063fdcc48460931f2c9d53022` |
| `data_e.cpk::battle/boss/e20A.bin` | 3 | `3b1fd98cf35a8aa12f60754749a17631ca2f0b0e2ffd48af36e2d0695c1fea42` |
| `data_e.cpk::battle/boss/e20B.bin` | 6 | `7094f69a01648e058ff2b3915fb404de66307c6cf9b3bcf46480e6d687ee48e6` |
| `data_e.cpk::battle/boss/e20C.bin` | 2 | `72c404d34441726d36baaef04851b86cff34b71a1e1b00f27c9ff99d439b0c01` |
| `data_e.cpk::battle/boss/e20D.bin` | 4 | `e6855a8d9e8fb806e7406fc8dfc0518b93f26523d058f4375ca090c70d055edb` |
| `data_e.cpk::battle/boss/e214.bin` | 2 | `1be9e941cb82211972ab99687d9de1ed20bdea6029570d6134619be944e6a4ad` |
| `data_e.cpk::battle/boss/e215.bin` | 14 | `e5311155547af85cd9f9d96ffa17a7b45a827a0bc9f7929003f3ed9135505383` |
| `data_e.cpk::battle/boss/e216.bin` | 2 | `fed2851c72e4462b35764d7eed30c7d021774bb04282a7bf535a29540fe0d277` |
| `data_e.cpk::battle/boss/e217.bin` | 4 | `45695293c28514dc8ca5b6a51721e8f7dbaa7f475a3290245cfb68ac85604d5e` |
| `data_e.cpk::battle/btleff.bin` | 16 | `2965bf760b8edf74136d3a7509b8b7c982a5954d4d3bbe277535cf9c26eac232` |
| `data_e.cpk::event/fishing.bin` | 3 | `985681b949fb09b4f700609f4a5ba7427aa55daf3a2e2a36dcbec1738ad4a804` |
| `data_e.cpk::event_data/community.bin` | 4 | `483edeb1aec232a6ebe4d89f3ce510f2546c9510e7805e4149cba6ec1b337f8d` |
| `data_e.cpk::init.bin::event/icon.bin` | 3 | `525beee6c0d5c61979aee4250a08a33f2c27e2845d98defcaccdd4d9be3cc553` |
| `data_e.cpk::skill/100.pac` | 8 | `63e9206ac23e4c1e97bcdd17b986e0aaaa50dabfa8cd0d4c360ea855e0601905` |
| `data_e.cpk::skill/101.pac` | 12 | `c4b3709bd7f893d7ff99c2be3e54e3a64e5da5aa527b27ecf4ef4d0af825cc5c` |
| `data_e.cpk::skill/102.pac` | 9 | `30d4b734f74a2f2be1038e7961b201f9adc9105e8a1903b8522e4e281e9068fd` |
| `data_e.cpk::skill/11C.pac` | 14 | `15206a3424b9b7db672652646ba77a810fc602931803a20744a24f7d0d9879dd` |
| `data_e.cpk::skill/172.pac` | 2 | `00fc943b7cc633475ac5c6727187b57831f11d8d75a92f5a93adb7dc57a9c522` |
| `data_e.cpk::skill/173.pac` | 1 | `3c47360f02aa9707fe889d3389ce9062fb9dcb360372fe58697b5aafc757ccba` |
| `data_e.cpk::skill/174.pac` | 1 | `d539d58f315a43a365f5e180331300d8cbe993566c2d627a4307f31694b7a517` |
| `data_e.cpk::skill/175.pac` | 1 | `b1f1a64f5420bcee1b935be003f54c751f54b6f23dd85a5918e3603ebb00a14d` |
| `data_e.cpk::skill/176.pac` | 1 | `3aae39bf46bcda2eab9c8134c35e331d80bcb47cf65cc583bf4155117aa7296f` |
| `data_e.cpk::skill/177.pac` | 1 | `aeef4cc7cb28a2c6ce1aef7a255c908f2e5143e27099582d0d05dc28be5d6b51` |
| `data_e.cpk::skill/18E.pac` | 9 | `def3ae201fcf05e49032192de4cea9bbe1ad4743e9e3b9b0c20c6b4e48ea28dd` |
| `data_e.cpk::skill/18F.pac` | 12 | `d8602f86bc694ef6057962fe85f2d195823ee7020a6c56597569457fc9ca96b1` |
| `data_es.cpk::battle/battle_pack.bin::btleff.bin` | 16 | `2681670bae7cd96d3031b8b5fd015a06d4955bef2db0b14169b35a27cf8cfb74` |
| `data_es.cpk::battle/battle_pack2.bin::btleff.bin` | 16 | `dec3d6ec198c7b0fed30686cae467a2a8aa8698b8d32e8f184fa68a0b2274005` |
| `data_es.cpk::battle/boss/e200.bin` | 5 | `a29408989edff155addce178e92fed9553bfaf047bbdbab118c8c3593d262ba0` |
| `data_es.cpk::battle/boss/e201.bin` | 3 | `4e3701d7242e34dc2a98cd7f385f3170cbe4fd668a70686a0eef9538f24ac17a` |
| `data_es.cpk::battle/boss/e202.bin` | 2 | `d333834561b0d0049f1afd8e2e0de031558966c5771ab45775e912ff56f5cfbb` |
| `data_es.cpk::battle/boss/e203.bin` | 5 | `86dc6a5dc3fea03e99af095cb5366add690042160cd5d7474b3aad87b54725be` |
| `data_es.cpk::battle/boss/e204.bin` | 3 | `01ae2d7b299922774a920e359bb596dafd8b41f89c5c66f5fb24fa64478d8fc0` |
| `data_es.cpk::battle/boss/e205.bin` | 5 | `8b7b17add75e35e7b3cb76b3e8d58189fd99166ca8e9263b8a1fc1fa2630cb70` |
| `data_es.cpk::battle/boss/e206.bin` | 6 | `e67d57d4a7aedd6cfc36596d8540e072bec4134670da76563acf71fd1b2dc7c8` |
| `data_es.cpk::battle/boss/e207.bin` | 2 | `f3f5d637452b9864b05c9f9275ef7441010bdcf13bb2ec77f0833e1abee2f5cc` |
| `data_es.cpk::battle/boss/e208.bin` | 5 | `7ccc6994d0fe7f7f13d325ef3e83a6ab69c1c0c2570539a224f689c937700036` |
| `data_es.cpk::battle/boss/e209.bin` | 5 | `29bc471401468909d1e56a3bf098d09a3253aa15e11e352604b30d1d52265baf` |
| `data_es.cpk::battle/boss/e20A.bin` | 3 | `480d986af8ba74d2c6593ba773c51a0af74a76ae57f5fa0fa8f616f178db64b3` |
| `data_es.cpk::battle/boss/e20B.bin` | 6 | `40e8da4ffd51b8bdd348155a136f6e32fafa5abbd46271fd7c37ae9c95b8fc08` |
| `data_es.cpk::battle/boss/e20C.bin` | 2 | `f1b9a25f044e1b4ab205d6ea6d440ab4566bf75e5103599d3efaee85118f0534` |
| `data_es.cpk::battle/boss/e20D.bin` | 4 | `932bf06d51934b638d08667f0656052a312af085254debf45c5a16c5f8abd3dc` |
| `data_es.cpk::battle/boss/e214.bin` | 2 | `10aab72780117eb9024a101b21410fd81904726c89fe72efa0114230a33dd88b` |
| `data_es.cpk::battle/boss/e215.bin` | 14 | `da89d08b356bf0b48ef4d638af2587ed2fe47c08946c2f2b58cd31db755673c2` |
| `data_es.cpk::battle/boss/e216.bin` | 2 | `857f44844f37efbebfe1c33ed571fbfaa4cbd6241227d2f407f1175b55844522` |
| `data_es.cpk::battle/boss/e217.bin` | 4 | `7107a56c259fe00bdedf18aeeae46f4e5c9dd8eeaedaeaed3eedb1db52e25cdd` |
| `data_es.cpk::event/fishing.bin` | 3 | `f09cbcbee125f44fe9d2d280103b1800b3c29924417a26bdc188a863f67f8972` |
| `data_es.cpk::event_data/community.bin` | 4 | `6fe85581f00d96465abd8814bb4e1b855324b58c07a8a64977750fabadf91498` |
| `data_es.cpk::init.bin::event/icon.bin` | 3 | `08bd7755046e682d68db090bf04ed1e756161e27bf2e13769cd26db8077484d6` |
| `data_fr.cpk::battle/battle_pack.bin::btleff.bin` | 16 | `2f0dd54a1ddfa39685e95af3712b97a106cee52ee09e1bf60e38e741e6f64122` |
| `data_fr.cpk::battle/battle_pack2.bin::btleff.bin` | 16 | `39c244728d0eb0e9d631bf7c8aabb3c22def6eb5a36af05581c2ed106a6014ef` |
| `data_fr.cpk::battle/boss/e200.bin` | 5 | `a42d431297299e5c97ca11e8141d4d33b47d09eb2de9cfc613efcd73c11937cd` |
| `data_fr.cpk::battle/boss/e201.bin` | 3 | `f3f02ac27b24f183bd1bf7cb7cd70481c664a4a5eb33d6b71f0fd7e01d8a58cf` |
| `data_fr.cpk::battle/boss/e202.bin` | 2 | `171ba5502327ed6067465493b17771f7f7f28d11e31744b65795198b8359e219` |
| `data_fr.cpk::battle/boss/e203.bin` | 5 | `760d4b9e96edaaeee3af1fc1e79b0c8ee94deaf241533f260b6562a9d13552f6` |
| `data_fr.cpk::battle/boss/e204.bin` | 3 | `aacc7932eeb5b2b3e1913e94718881983d2438eaa0a0e1a402e9e35e32e9c2a1` |
| `data_fr.cpk::battle/boss/e205.bin` | 5 | `d6bdffd8ca9197eb60711fc0fa8b6c27a2fe905275b92c706c459f11db23bd2b` |
| `data_fr.cpk::battle/boss/e206.bin` | 6 | `0713e1b645ea56def48b57720363f6721d8633144e52c6d942e0c536b69c0193` |
| `data_fr.cpk::battle/boss/e207.bin` | 2 | `04d3616c2bb362a85422fbd32a2cd3c81a79e6b3d7d5a0290345b68141b3bc87` |
| `data_fr.cpk::battle/boss/e208.bin` | 5 | `c83796afd91bfba771eef1a461a01e3b898fbb55e83bb6e0faa69370dd71f052` |
| `data_fr.cpk::battle/boss/e209.bin` | 5 | `0eb4aa55b2376ccc616973c4b4955e3fd0e806d769dd4b5fa9f3f1bb7d9c0963` |
| `data_fr.cpk::battle/boss/e20A.bin` | 3 | `8ec89d926d4e1813cd361b7d9350a6db95ff5c2f176eaaf16d805dbcbb605360` |
| `data_fr.cpk::battle/boss/e20B.bin` | 6 | `707e1f4c34b34de3a05e7aa948205af8e876d36f2cf47fbf5b89544a88e134c4` |
| `data_fr.cpk::battle/boss/e20C.bin` | 2 | `90d30ed40e507cc20eb9d4dd6d58bd6830b172f43d01b076c45949e041f1eb2a` |
| `data_fr.cpk::battle/boss/e20D.bin` | 4 | `61fae5fcbc3977589bafe62b03ced5252bb27f87023a3a6fe5b45ef204c1494d` |
| `data_fr.cpk::battle/boss/e214.bin` | 2 | `e39b629fa982eb58f388fac32492eb6ec1645d7b9108724a25e3e4e59bf5172c` |
| `data_fr.cpk::battle/boss/e215.bin` | 14 | `a4928dcb81264471e8ce16ac4fb618d95e4edae1960b60725ea959e3f47d07ff` |
| `data_fr.cpk::battle/boss/e216.bin` | 2 | `f80ad821f2695df6b9d63fbf1df9920907ceee45372c789baafc1263a74ef0ef` |
| `data_fr.cpk::battle/boss/e217.bin` | 4 | `3192aeed14f622f7d303c89b5e72a2a42e947c1750f4ff761d9af75b1fdf1e78` |
| `data_fr.cpk::event/fishing.bin` | 3 | `c736c9e140dcefbc5b8f2ec447e0080b1f87a3c4710f632ad88090483d0b4e17` |
| `data_fr.cpk::event_data/community.bin` | 4 | `5f4b912bf2c63849bd13ccd558351262890adc0bfad7c0202ba4f2e0fb8c0c79` |
| `data_fr.cpk::init.bin::event/icon.bin` | 3 | `2c9b9539f914faa4f9b89a551f87d4121eaaef8639c46b4f09861749b1f0d6b3` |
| `data_it.cpk::battle/battle_pack.bin::btleff.bin` | 16 | `a5f93c43682ce178e32caedd2cc2bbac8b9dddbd499917ab25275e36f9c87b1e` |
| `data_it.cpk::battle/battle_pack2.bin::btleff.bin` | 16 | `1879dfcae4051b560b9177f2f0d9c52d1a35eccc9e304ff41027ba3793571810` |
| `data_it.cpk::battle/boss/e200.bin` | 5 | `34e7f07ec017011fc798dd9679db8a4e571a3d27248ab637c8bf3b300140793e` |
| `data_it.cpk::battle/boss/e201.bin` | 3 | `d1bca971048f2725e383bb87fb1e2491f5f69b000ce3faf9b7a4445d44f0f60b` |
| `data_it.cpk::battle/boss/e202.bin` | 2 | `5cfe37d7399d2c0af7ce5edeb4c9ee2d64e3159b97a5beff18dc2f2c27f02a10` |
| `data_it.cpk::battle/boss/e203.bin` | 5 | `53575741d1e0059953da31ce38da94d90ee979cdff4a69ef3795e099b5086f85` |
| `data_it.cpk::battle/boss/e204.bin` | 3 | `6371c14a7d803425056bcda93205a0caa90bd12ea9959753db7640272e967b35` |
| `data_it.cpk::battle/boss/e205.bin` | 5 | `199bec512fbd6f8499934a415a38f158ac091fd0e6291fb11f1d40cb739183f3` |
| `data_it.cpk::battle/boss/e206.bin` | 6 | `7f6f41ceb85a41e1d556d57bef1822f51cadb6d5892e9f201c97c9cf77ceadf6` |
| `data_it.cpk::battle/boss/e207.bin` | 2 | `81b57a41c86e37e1fc0ceff8e886f0febb93ba7da49e8086dbcddecc29abeda1` |
| `data_it.cpk::battle/boss/e208.bin` | 5 | `8be110447d1baf75cc215609ddd0b672cceb1383f719b1f264cd61403a2973a1` |
| `data_it.cpk::battle/boss/e209.bin` | 5 | `68298f10e9a5e83a82a3eebbad2622969f9d961f695c5646694a2daf522d2daa` |
| `data_it.cpk::battle/boss/e20A.bin` | 3 | `07b07b40b6822fc1fef5d22193230f013ece729630e1085d3f30ea555316b854` |
| `data_it.cpk::battle/boss/e20B.bin` | 6 | `45a3dedb937ac38cac1dee759f4dde9c16314ce38823846e07b3085e0ed3b07f` |
| `data_it.cpk::battle/boss/e20C.bin` | 2 | `6f10934960329922a2bf0fd91acd376b4a6702d4c104e4d8eba44a58d550d44e` |
| `data_it.cpk::battle/boss/e20D.bin` | 4 | `acff6662bef5bead69bf07c910f4e5e1b3d6983b50fc9e12d3907c06d426a3f0` |
| `data_it.cpk::battle/boss/e214.bin` | 2 | `eee945abc2bd6bac696e135034129fb67d14bd60a5e8807898c48b01547a679d` |
| `data_it.cpk::battle/boss/e215.bin` | 14 | `d4e0c85895df14cedf14b466d6a10ae60c5bd91a24dd68ee4f3a1be536c2121b` |
| `data_it.cpk::battle/boss/e216.bin` | 2 | `836258755db589b38052843f1f6e036ad3ecac230ded083fa6f6d17c13e6f8cf` |
| `data_it.cpk::battle/boss/e217.bin` | 4 | `dad7a0d8866772a6af19a2ea8b97fa48a2c3b728a4239fb8670fd9648503c3f0` |
| `data_it.cpk::event/fishing.bin` | 3 | `a6115cdce53bd76f27ce699dad81d84f89578fcbde9c71435eca2a8e6e4173f7` |
| `data_it.cpk::event_data/community.bin` | 4 | `afcf1aadc976459a350bd150a7a28ea4e1da21e8dc83a306e7344119fd7f2f5f` |
| `data_it.cpk::init.bin::event/icon.bin` | 3 | `ded0f50873964dedbdf727722814f1633733f3cb5b35a4af1dd093b8e329aa4c` |
| `data_k.cpk` | 36 | `2744bcb5a2d8431c185004ac55e1056b40e93ce36bbaa407a56b0d456dbac74c` |
| `data_k.cpk::battle/battle_pack.bin::btleff.bin` | 16 | `b8364d83ac5fb15fb17b667975ad0d8b660fd0a56e3399e94c5a9bfd748239af` |
| `data_k.cpk::battle/battle_pack2.bin::btleff.bin` | 16 | `28d69cfd30d69c9e00e631515750d267517a2b22e52f0ad83d4caf715a6f4bb8` |
| `data_k.cpk::battle/boss/e200.bin` | 5 | `548737b4e9f0e2c0a166fc55e99e0481ca03c8f355cd308b34836531643f3e62` |
| `data_k.cpk::battle/boss/e201.bin` | 3 | `c30f6bbb3b7ffde23686316faf0d389cbca4b1273ff35a20a4b92867b72eb5f7` |
| `data_k.cpk::battle/boss/e202.bin` | 2 | `e09d3591cc567b827e6203d56f99bf40a0b91cc4b43a96df461155bfdb34dc1b` |
| `data_k.cpk::battle/boss/e203.bin` | 5 | `82498e3ad9752edfe828889c76bbb9007faea9ceff0df439867d34ff139fc880` |
| `data_k.cpk::battle/boss/e204.bin` | 3 | `8b738ca8250421eddabc7b87fe074cbeafc50f3fff14792024efaf0ea6bad4ae` |
| `data_k.cpk::battle/boss/e205.bin` | 5 | `2d30a43e92b2de7ef21122655d0b0f022b1ca2f5963e76b3972c1c697c322a98` |
| `data_k.cpk::battle/boss/e206.bin` | 6 | `7f1b3931d1a53208ad241bba863fa643e43ce0e2f222a4fb25a235794e752d43` |
| `data_k.cpk::battle/boss/e207.bin` | 2 | `4603bce29c507f15f5ef2f2f322f7580e40510d66629331fc033fa0f78c63c3e` |
| `data_k.cpk::battle/boss/e208.bin` | 5 | `677c0043719b765e9437a685475fb897b87b11439c543dec3493ed347b286d9a` |
| `data_k.cpk::battle/boss/e209.bin` | 5 | `2bb4710e08acb159325ca69536008326945d347245d95b7715d7ac1053a14e46` |
| `data_k.cpk::battle/boss/e20A.bin` | 3 | `8fce65785cf07b29f51ce73e2dda707d7c21ff43a4264496e7fe5cbd6b23a966` |
| `data_k.cpk::battle/boss/e20B.bin` | 6 | `3f179aeedc8b540feff3c6426a5e61787b0c70620df40797b02aa6c9a9066313` |
| `data_k.cpk::battle/boss/e20C.bin` | 2 | `48dad2de7b6898dfe1507aba5e2d207f397117c289080e8ed1db8adaba8332b9` |
| `data_k.cpk::battle/boss/e20D.bin` | 4 | `8c742fccafc5e1304d4295c317bb40f976239a8c23be4659212120c30d0a88c9` |
| `data_k.cpk::battle/boss/e214.bin` | 2 | `823f68283ce7195592f2a76aad37c6ce3cf7ef20a23f58f39d3748a7ef17a7e6` |
| `data_k.cpk::battle/boss/e215.bin` | 14 | `01e8e70d9b32b537056804551c45ee3f9f1b2e18bef229607e5ffacd6ed5d026` |
| `data_k.cpk::battle/boss/e216.bin` | 2 | `dbed4e85433b2074831fc0a4b151c937f844b737d5387fb035d77226c322b7a5` |
| `data_k.cpk::battle/boss/e217.bin` | 4 | `e4467a470c95346dc47249e104ac0d9ee69b4d0c580987434d69430776e2f82d` |
| `data_k.cpk::battle/btleff.bin` | 16 | `d06cb00cfb2b1821c848c94604185ce912f5c075c31d32add03fca88b9392cb4` |
| `data_k.cpk::event/fishing.bin` | 3 | `e843ae6b6c832625b6ae88af5c086fc1596eecf140434577b9832d9d264c1dbf` |
| `data_k.cpk::event_data/community.bin` | 4 | `3945a675aef57775de5102cb5999ceaa01a3dc21050f772cea91df1ee1e9fa15` |
| `data_k.cpk::init.bin::event/icon.bin` | 3 | `63735320b1ade266953383c92726b6f4456c544d4182b54cd387223264bcc75c` |
| `data_k.cpk::skill/100.pac` | 8 | `cba6cd1391c956f25b88b98f2f72054f69c2fe16ce5721051b95eee54f59f44a` |
| `data_k.cpk::skill/101.pac` | 12 | `1d469ede4d2887c087e9f40927fcd55479a685cdce3e0c2e7204bc1aaebe6f7f` |
| `data_k.cpk::skill/102.pac` | 9 | `386a7b5793cd1d88c6d021811c1edd8ecf780ceda597623ed2615556f3016694` |
| `data_k.cpk::skill/11C.pac` | 14 | `66183d570e65f55835997e3b87342f3e910b7a7ce26a5bed2ae0cc4608c7d026` |
| `data_k.cpk::skill/172.pac` | 2 | `c8d49bba295f6c381828bfdabafdede65ef9bf87587287b25122600cd8fd233a` |
| `data_k.cpk::skill/173.pac` | 1 | `d6ae83863fc99f09958002c06c1a4e232b58833d1d198d3afde0c12bd16cc8af` |
| `data_k.cpk::skill/174.pac` | 1 | `3997d9dfa9a6f35ffb3b4e190bf0b123e3586c9fc0b1c9c46206a59ce8329577` |
| `data_k.cpk::skill/175.pac` | 1 | `b8b42c797ccba352b79ce12b0c8447d58c5164204549d39ff4facd5f2e935448` |
| `data_k.cpk::skill/176.pac` | 1 | `4393cb62878c24adcb56b2010fb0d0af15def6f51b59df122eb3e41cbd411f16` |
| `data_k.cpk::skill/177.pac` | 1 | `b7bea48dce4ea2210961f7fc67610e67e86398361f7fb8c1bf65f6e7c2732e42` |
| `data_k.cpk::skill/18E.pac` | 9 | `4eb7684ba65e0d7f6a321e53aadc45c03966ff1fb2256dc68dbf5eedba3c8182` |
| `data_k.cpk::skill/18F.pac` | 12 | `ac5773dc3df50d90f34bf5516718b16b97308b9539626399606c0c93741ef91a` |

## Complete payload identity hashes

Each example is a complete root asset path plus root-relative byte offset(hex). Occurrences count distinct physical descriptor addresses within every logical root path. Part clone/shared-data aliases do not multiply the same physical descriptor.

| Family | Bytes(decimal) | SHA-256 | Occurrences | First example/offset |
|---|---:|---|---:|---|
| AMD-CHNK | 134208 | `072aadd4fb22a5d8b09538f0b23c0bb128d76918152035fc41fe017058540e00` | 2 | `data.cpk::battle/cutin/syoudaku/BOKO_SD_7_W.EPL` +`0x2650` |
| AMD-CHNK | 72480 | `07a8708993946caaf772783c19647bbe1035156da3e111886945cbf29b350170` | 2 | `data.cpk::battle/cutin/boko/koma3/3KOMA_A_1_S.EPL` +`0x2E0` |
| AMD-CHNK | 358368 | `0b913e75a6666b1e6131ea729c4472fcb7b261559c365914f1507c324f379265` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_B_7_W.EPL` +`0x220` |
| AMD-CHNK | 186552 | `0e40fd36aaab84d9d3d66a5bdcd5d855cd1a62a827de4212aae99160eac9367f` | 1 | `data.cpk::skill/15D.pac::15D.BED` +`0xFF20` |
| AMD-CHNK | 333952 | `0ef19b7b69d6b3c9f37e9472880dfc080aa2d30b75214736382e1669497dc8b1` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_B_2_S.EPL` +`0x220` |
| AMD-CHNK | 476832 | `0f5617cb4073aad334d9f1b73bba828a94ff0ad50f6f373e6201006491a3dc4c` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_C_4_S.EPL` +`0x220` |
| AMD-CHNK | 334208 | `0fbdb1b1d2a8ecbb7981fd398a3f06513cad45857cd5fb590f89a1316782aa74` | 1 | `data.cpk::battle/cutin/boko/koma2/2KOMA_B_2_S.EPL` +`0x220` |
| AMD-CHNK | 412480 | `1231f339a818f66af78f58a4aa5811224ec05f75a42f04333be7d709c84d3f6c` | 1 | `data.cpk::battle/bike/B116_W.BED` +`0x1070` |
| AMD-CHNK | 16256 | `1637183bfca62abcbebe87a2ffa179fe41ebda27118c5c758685242847a6de24` | 2 | `data.cpk::battle/cutin/tokusyu/TOKUS_5_S.EPL` +`0x520` |
| AMD-CHNK | 657152 | `1738c5b51d8b4aa4bee548993f4628395281ca29061c9f0ddb51359d2962c48e` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_B_4_W.EPL` +`0x220` |
| AMD-CHNK | 334240 | `1a675e37aacb13045259cefc1e6030d649afbeec755d6767ebb1e4166681252a` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_D_2_S.EPL` +`0x220` |
| AMD-CHNK | 108032 | `1b5a264145c22558bbb60456436b377fe9d7e24d89945c26ddd00fd9ae0b15a8` | 4 | `data.cpk::battle/cutin/syoukan/SYKN_1.EPL` +`0x2E0` |
| AMD-CHNK | 108544 | `1e931ecccd96bd1aa8595165bc744685c150b917d3c86f5f64f48e290a38e99d` | 4 | `data.cpk::battle/cutin/syoukan/SYKN_4.EPL` +`0x2E0` |
| AMD-CHNK | 134208 | `1ef692958079ae62df6fede9eeecb88aef0e8d3339e2f968e8dccbc85a635b11` | 2 | `data.cpk::battle/cutin/syoudaku/BOKO_SD_6_S.EPL` +`0x2650` |
| AMD-CHNK | 1632 | `20127959554573997d2f494a7dd1dda11f2598e6b7989ff643703b2649f0befb` | 4 | `data_k.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z_J.BED` +`0x950` |
| AMD-CHNK | 134208 | `21142176ab216ae2f9514989a8485fa7fa6395bf14122b68832ae6d2bd776998` | 2 | `data.cpk::battle/cutin/syoudaku/BOKO_SD_3_S.EPL` +`0x2650` |
| AMD-CHNK | 387136 | `22ad3ad15a91c468167c31694f7c988669d8b4de30ef126a5670240adffd3064` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_C_3_W.EPL` +`0x220` |
| AMD-CHNK | 2400 | `23242ad8277e60d52d19362feecae7452c91231832f46e8ebc9c23f94b26a592` | 4 | `data_e.cpk::battle/battle_pack.bin::btleff.bin::ATC_D_J.BED` +`0x950` |
| AMD-CHNK | 438176 | `26017f55f00601782e85d93d06d93dc78e9f979ecc8ee21b1e9c6ea83eaf6227` | 1 | `data.cpk::battle/bike/B11B_W.BED` +`0x46D0` |
| AMD-CHNK | 381696 | `27b1af9426d4f894a0f2848c688503b714acf697a8bfd10aca0a46ec08851b03` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_B_6_S.EPL` +`0x220` |
| AMD-CHNK | 381632 | `27e8b9ab90ca5e01d53752805aa7f3c563878bc4fb29b60f83ce8a083cb9ea12` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_B_6_S.EPL` +`0x220` |
| AMD-CHNK | 653504 | `2894043dd64424542e89baf4909f567fe55979d0ca65571773364324b62afeff` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_B_4_W.EPL` +`0x220` |
| AMD-CHNK | 355072 | `28af807ac0045c6e19d1c7af737ea56a487943c8f05a5202a83634c889188de7` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_D_7_W.EPL` +`0x220` |
| AMD-CHNK | 4160 | `2a17ff5b74fe14b38ebd8c7c865b1109bbe203fb375475f2448ede3f18153103` | 16 | `data.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z_J.BED` +`0x950` |
| AMD-CHNK | 319904 | `31bd45eb745177ea76905b65989ffeef956fd2aa99511b6a86ab09a8b9f8ce4b` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_B_2_W.EPL` +`0x220` |
| AMD-CHNK | 134176 | `336142882f4e830a63ff7b3995530cbadff14ac4ceac05f28b4c451d9c41beef` | 4 | `data.cpk::battle/cutin/syoudaku/BOKO_SD_8_S.EPL` +`0x2650` |
| AMD-CHNK | 312952 | `39b278b30b23c7f2ae449bf2c3921fa2c54ba5d63a96e01e7c86b1e54db23dbc` | 1 | `data.cpk::battle/assist/ASSIST_P002.BED` +`0x820` |
| AMD-CHNK | 265888 | `3a9fc5119d1acdb7d6d9a54d799585984c00e5bedae7c7ef6e4eab3a0ab7263f` | 1 | `data.cpk::battle/cutin/tokusyu/TOKUS_5_S.EPL` +`0x45840` |
| AMD-CHNK | 387424 | `3b7505e704fa02f3b10394fec610319f9ae5ecf2d3855e4ec887816399410ce0` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_B_3_W.EPL` +`0x220` |
| AMD-CHNK | 328064 | `3dcaf699f0e7b9f9f05c344c8300583664c9977af191103fba97bcbeb17b61f6` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_A_1_S.EPL` +`0x11ED0` |
| AMD-CHNK | 320832 | `41c66135a65e5e2bd3269329ce34c55c34fc83e9d092e3fd15d41444bd159b11` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_C_2_W.EPL` +`0x220` |
| AMD-CHNK | 372224 | `42cb763b21f4a0380d110557c1dc1a69fb1937db663434a1cd77ca4bfbec451e` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_C_3_S.EPL` +`0x220` |
| AMD-CHNK | 654240 | `4913413e73e9e3d274b86ddea1364f803a006bd52b1c15de1690258c309fcebf` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_D_4_W.EPL` +`0x220` |
| AMD-CHNK | 107168 | `4b3034f827dd31084606ba70f54797170dcdb3ffdc4757ec1c526ada1594fa05` | 4 | `data.cpk::battle/cutin/syoukan/SYKN_7.EPL` +`0x2E0` |
| AMD-CHNK | 383296 | `4c4a8d9e097018b683f8f24ddee5b177dc42d5f376538e06bc99af0eb6d5e8b5` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_A_1_W.EPL` +`0x11ED0` |
| AMD-CHNK | 419200 | `4ca0a5e6fb23a5149e1244479c73779be98873868bbfc52641d4b2cfe3362ab6` | 1 | `data.cpk::battle/cutin/boko/koma2/2KOMA_B_3_W.EPL` +`0x220` |
| AMD-CHNK | 390304 | `4f68fa74365582b95515a9e1b22a00a048205abc8e00eff675b1b8c83183b9a7` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_D_3_S.EPL` +`0x220` |
| AMD-CHNK | 134208 | `5064b61ad94d8f28ab6a49d1b6c2ef1e674ff3535aae869998b184c9dd534ea3` | 2 | `data.cpk::battle/cutin/syoudaku/BOKO_SD_6_W.EPL` +`0x2650` |
| AMD-CHNK | 134208 | `5141cc2f080056a9b10cc7610010da44513a3c72d2f309fba88065b06853458e` | 2 | `data.cpk::battle/cutin/syoudaku/BOKO_SD_2_S.EPL` +`0x2650` |
| AMD-CHNK | 387520 | `57e4ba87084d8e26967739a1ee8f1568a613ecf79566b537b0dc2473f93f1667` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_B_3_W.EPL` +`0x220` |
| AMD-CHNK | 359168 | `591431ad8fe4faab138c7686e0fbd4b6af0677779dc6093f52990047d318ba0e` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_C_7_W.EPL` +`0x220` |
| AMD-CHNK | 226328 | `596d8a0af781f7cc765fafc03af6d46587f1d74d5a179719c79515af350027c6` | 1 | `data.cpk::battle/assist/ASSIST_P000.BED` +`0x820` |
| AMD-CHNK | 387328 | `5adfaf8592650bf39b9f9838b5f89d1d26652a118311ff14793c380d22b62585` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_C_7_S.EPL` +`0x220` |
| AMD-CHNK | 398464 | `5aed36bf505df90e0829012c05441c3cc52bbfb13e1b2ec5c6601f219a54f0cc` | 1 | `data.cpk::battle/bike/B119_W.BED` +`0x1A5B0` |
| AMD-CHNK | 380640 | `5c85b9d8325c6f9ed86585af339c5eeca1609380f2dc516f4638e0388f7bd1c8` | 1 | `data.cpk::battle/cutin/boko/koma2/2KOMA_B_6_W.EPL` +`0x220` |
| AMD-CHNK | 444256 | `5d61122ff08630ace7777e1e3997eccc57b0a7ca7447be318eefaf5a09eb4874` | 1 | `data.cpk::battle/cutin/boko/koma2/2KOMA_B_4_W.EPL` +`0x220` |
| AMD-CHNK | 380896 | `5d7f848363287673dff03747b91d70132758e0e15dfbafe77d9603b9890faa34` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_D_6_S.EPL` +`0x220` |
| AMD-CHNK | 358464 | `615cfd0db975517610d7443171629e335e76eb1bd35ad55f42a4406173f78926` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_B_7_W.EPL` +`0x220` |
| AMD-CHNK | 240736 | `61916d662869a4b4fd1a947415bf478fbdfb73de20642b9d452d6b61bf67bedc` | 2 | `data.cpk::battle/bike/B11A_S.BED` +`0x1760` |
| AMD-CHNK | 367840 | `63522fd5ae8d51cfff4c5a653f83d64dfa8f4474b862edb57e9c412ac3e84a1c` | 2 | `data.cpk::battle/cutin/boko/koma4/4KOMA_C_8_S.EPL` +`0x220` |
| AMD-CHNK | 377376 | `6a5c79e2f8ab7836358e346f0e002738378bd28dd140e5c25bf6bbb3afad5af0` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_A_1_W.EPL` +`0x12B70` |
| AMD-CHNK | 2176 | `6b8eb556243382a0189e1298a5431d77ec295193e56e26832d2e445960f03bb6` | 4 | `data_e.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z_J.BED` +`0x950` |
| AMD-CHNK | 391936 | `6d28cfea5dbce6f7dac82a99c1610f741e863808b3dc4e663b464b6ab8cf31c7` | 1 | `data.cpk::battle/cutin/boko/koma2/2KOMA_B_4_S.EPL` +`0x220` |
| AMD-CHNK | 385344 | `6db097e022162af6b8957921753527c4a895114c52ab6d93157462f26cf06e2d` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_D_3_W.EPL` +`0x220` |
| AMD-CHNK | 372928 | `6e3841baf2a87c6cdb09528fe51f9753b53684aaf48ab55eaf5a1aca669a294e` | 1 | `data.cpk::battle/bike/B118_W.BED` +`0xE9D0` |
| AMD-CHNK | 43360 | `718af99c7189a61d53905be0031f96f24a9db09a856cb5444728c1006dd56790` | 1 | `data.cpk::battle/cutin/boko/koma2/2KOMA_A_1_S.EPL` +`0x2E0` |
| AMD-CHNK | 387520 | `7668df2958d13c515959d5fa2745fa24578ae3c769dfd541fcb98414db4c7851` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_B_7_S.EPL` +`0x220` |
| AMD-CHNK | 391136 | `79c54e0f36ef6df4e0eca0be1150560bb078e065ce8c07a0958aab16a94e12ac` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_C_3_S.EPL` +`0x220` |
| AMD-CHNK | 322144 | `79d2267a9bf0fb0e86eda6abddba19b79545d2189b5d7d3607b0b78a11278da6` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_A_1_S.EPL` +`0x12B70` |
| AMD-CHNK | 477248 | `7e8cf4e82ebacc15c51871c9acd3eba28d3c2af778925e98c0d47bdbaf2bd88c` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_B_4_S.EPL` +`0x220` |
| AMD-CHNK | 316800 | `7ff660fb247dd0f35709b26bcec4c676611583e4deff0eb08c7d646f69f76116` | 1 | `data.cpk::battle/cutin/boko/koma2/2KOMA_A_1_S.EPL` +`0xAD10` |
| AMD-CHNK | 365376 | `80e827821c36faee587e465be9c50e4ac67b62ec33fce1a4b6e11ab82d223a09` | 2 | `data.cpk::battle/cutin/boko/koma2/2KOMA_B_8_S.EPL` +`0x220` |
| AMD-CHNK | 265856 | `82cab3f0d8936f74a78a35904da24bc0e420a826960eb0e137444fc89b280bb2` | 1 | `data.cpk::battle/cutin/tokusyu/TOKUS_5_W.EPL` +`0x45840` |
| AMD-CHNK | 416704 | `84a527e1808f1eed7971dc1a61d86a681291844f9bb070ec82e6344c3a2f3c67` | 9 | `data.cpk::battle/boss/e215.bin::BED/e215_END.BED` +`0xC500` |
| AMD-CHNK | 384288 | `8523f26c9d3605c8526013714b6894cb3384e0cb06fc41febdc299b826868ef0` | 1 | `data.cpk::battle/cutin/boko/koma2/2KOMA_A_1_W.EPL` +`0x2E0` |
| AMD-CHNK | 167296 | `85796f5ce38917907e5ade1a874eace2c96554672000e96a2585e1060b784431` | 1 | `data.cpk::skill/15E.pac::15E.BED` +`0x3B300` |
| AMD-CHNK | 348032 | `85c671e54394a511b1dbbc05cf5bf59fb5697ee7bd9e0e456ae5bb493b4a9302` | 2 | `data.cpk::battle/cutin/boko/koma4/4KOMA_D_8_S.EPL` +`0x220` |
| AMD-CHNK | 653920 | `8c71902d729989c9af5c7c9b5a409f81fc18bf390e73dd3206bb4e3e7e3c4cd3` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_C_4_W.EPL` +`0x220` |
| AMD-CHNK | 266752 | `91212fd1e078a9c254dfe282eabb12a5369598bcca6e4942c7d73473cdec20e6` | 2 | `data.cpk::battle/cutin/tokusyu/TOKUS_5_S.EPL` +`0x4570` |
| AMD-CHNK | 366944 | `9187d6d4f6d48ab2c6d4ffe50eef0c98de2539a1f64986c259506429981af0bf` | 2 | `data.cpk::battle/cutin/boko/koma4/4KOMA_B_8_S.EPL` +`0x220` |
| AMD-CHNK | 365088 | `91c9273295b765c77acbc0aac20319905db3bfa2b969a2a9ee0fcac2c73aec67` | 1 | `data.cpk::battle/cutin/boko/koma2/2KOMA_B_7_S.EPL` +`0x220` |
| AMD-CHNK | 386432 | `9662f602b8d5d70f92ec3cd62aa74f7d7f9cf8bd0b306cd14ff46cf9ce1a103a` | 1 | `data.cpk::battle/bike/B117_S.BED` +`0x1340` |
| AMD-CHNK | 390880 | `9a9d92f683e5119e7a1c38f0f959c8cf24190741fe8a678456c8931148d18f01` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_B_3_S.EPL` +`0x220` |
| AMD-CHNK | 412704 | `9b8792de89117e5b3490904c4a5efae60cbd57aa685434fec6cdda6dbbf7ff42` | 1 | `data.cpk::battle/bike/B117_W.BED` +`0x1130` |
| AMD-CHNK | 383648 | `9d71c50c666f438fd73d68df30ba19a9768819f9994ff2bd3074af491651a099` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_B_6_W.EPL` +`0x220` |
| AMD-CHNK | 366656 | `9f560ad2f28885a447ce2df41cdbe9067b4fa49bd07d54544d82bcafcbf51d47` | 2 | `data.cpk::battle/cutin/boko/koma3/3KOMA_C_8_S.EPL` +`0x220` |
| AMD-CHNK | 1216 | `9f625a214026808404f2018eeadd764017b9b2597db61a4da894b8c427ae22c5` | 4 | `data_k.cpk::battle/battle_pack.bin::btleff.bin::ATC_D_J.BED` +`0x950` |
| AMD-CHNK | 380160 | `9fc2db32e88dea1fd74227a071093f36865897f2c8508c6bbded2a57bd61ae4f` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_D_6_W.EPL` +`0x220` |
| AMD-CHNK | 134208 | `a0983b759087dcab90d3ff285fe17d703556b62a0416fc1d1638c357aaacc7bd` | 2 | `data.cpk::battle/cutin/syoudaku/BOKO_SD_4_W.EPL` +`0x2650` |
| AMD-CHNK | 386464 | `a406e38d477a003631521b38393891fb629919c52080ecc797a0140bb2b28a58` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_B_7_S.EPL` +`0x220` |
| AMD-CHNK | 318784 | `a5cf164e8c0ed4caff5cf372bb990daad5499a2a62eeff463785de30232f10c4` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_D_2_W.EPL` +`0x220` |
| AMD-CHNK | 382496 | `a681496e8ff086437cf23406cdd5a5234fb5ec33b5b8750b98dce125f5332e54` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_D_7_S.EPL` +`0x220` |
| AMD-CHNK | 417440 | `a8f8e98e0da2aa9d6aeeec01dca322744782e23e7b7e95e949149d92a2a8c636` | 1 | `data.cpk::battle/bike/B11B_S.BED` +`0x46D0` |
| AMD-CHNK | 108096 | `a9779eab6311f06fcb8f3b73d75c9930db8d739d384930073abcb3ca46895819` | 4 | `data.cpk::battle/cutin/syoukan/SYKN_6.EPL` +`0x2E0` |
| AMD-CHNK | 134208 | `a98c443b18c75e9b1e9b34276da742f15c939d4ef7b7457a55387520c24c3e07` | 2 | `data.cpk::battle/cutin/syoudaku/BOKO_SD_3_W.EPL` +`0x2650` |
| AMD-CHNK | 374144 | `aab87c371f65bb5fba7ddde0f14646973f21891d1f71970b8b7c1761a3e9bc39` | 1 | `data.cpk::battle/cutin/boko/koma2/2KOMA_B_7_W.EPL` +`0x220` |
| AMD-CHNK | 363504 | `ab3294b8b5069a9a2ecfbe2204e76344359a06e061d90191f32a1fa0252a4360` | 9 | `data.cpk::battle/boss/e215.bin::BED/e215_END.BED` +`0xD3B80` |
| AMD-CHNK | 384384 | `ad87f0d7349e9478829b2be5ec8af82617836fbef6014365cc1945ce22e3950e` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_C_6_W.EPL` +`0x220` |
| AMD-CHNK | 387392 | `aee24b17d12b73f6d37422e6de0f52961200fe175c30b5cca29be4828e8a3234` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_C_3_W.EPL` +`0x220` |
| AMD-CHNK | 14656 | `b13070ffdd6517f03626af91b3a832c389b7eb0c0c1b40f9d1b2e531021affcb` | 2 | `data.cpk::battle/cutin/tokusyu/TOKUS_5_S.EPL` +`0xC7720` |
| AMD-CHNK | 386336 | `b15878b9f95e8d8f36859d21828b5499a269169e602ad74401064ce15ecfb38f` | 1 | `data.cpk::battle/bike/B116_S.BED` +`0x1070` |
| AMD-CHNK | 134208 | `b225b00ac5b1e2b8b2a1adbe286a609e8fd347317491904abd879fedb4c683f3` | 2 | `data.cpk::battle/cutin/syoudaku/BOKO_SD_7_S.EPL` +`0x2650` |
| AMD-CHNK | 265888 | `b29bac09f9e65c2edd5b891a07cab5301ab845b51a8a65da5dc42f882aed1e1c` | 1 | `data.cpk::battle/cutin/tokusyu/TOKUS_5_S.EPL` +`0x867B0` |
| AMD-CHNK | 1792 | `b6ccc2526672223f8ff957ebe0b5a6ee7b0a197712e6f8b378493a7ba7a9b5b5` | 4 | `data_e.cpk::battle/battle_pack.bin::btleff.bin::ATC_J_J.BED` +`0x950` |
| AMD-CHNK | 8160 | `b75f9f520967feb0262dc26c7b0db297a8d257af53198c4ed47dbe8129fa1567` | 13 | `data.cpk::battle/cutin/boko/koma2/2KOMA_B_5_S.EPL` +`0x220` |
| AMD-CHNK | 108128 | `bc32057761e36cf9df93a8b114121fa6e3ba16af3c0f4f9e5d263130f0ad486e` | 4 | `data.cpk::battle/cutin/syoukan/SYKN_2.EPL` +`0x2E0` |
| AMD-CHNK | 477440 | `bfb0887d3ebf656858a8e194765577f920bf81c222b8c8ae3596f01104de2350` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_D_4_S.EPL` +`0x220` |
| AMD-CHNK | 134208 | `c082f43e465d6ee8c50ca4bf56cf469cc9fefcbd9b24275724cd226141edd73a` | 2 | `data.cpk::battle/cutin/syoudaku/BOKO_SD_4_S.EPL` +`0x2650` |
| AMD-CHNK | 265888 | `c9c85af95338e4c5121c70052890cad5148f375ddd792541dbde4cac59b6ae06` | 1 | `data.cpk::battle/cutin/tokusyu/TOKUS_5_W.EPL` +`0x86790` |
| AMD-CHNK | 384608 | `cd92f2c54d2c4475231b406ebd2b58b838ad5c10435821834b5d7e10c2875b9f` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_C_6_W.EPL` +`0x220` |
| AMD-CHNK | 381536 | `cf16b82f92bd5236b2b7c1d28b2e9e3022f6691d33b5a988f2057ef6ed093338` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_C_6_S.EPL` +`0x220` |
| AMD-CHNK | 476448 | `cfbad86daa7641865bfc6e74b17e579b8cb9ca30a474d3f1e4ed310164d40816` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_B_4_S.EPL` +`0x220` |
| AMD-CHNK | 384128 | `d20b792c0df37e8bd0d3f4c2861770b39545379fda134f5843f5bde4acc5682f` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_B_6_W.EPL` +`0x220` |
| AMD-CHNK | 478464 | `d2b6422bf52d5a8a07357675d00c411d32b64ee9d6737fcaee7d6ff2a9100130` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_C_4_S.EPL` +`0x220` |
| AMD-CHNK | 358176 | `d84727f18c26688dadc33e3420754a944c8b9a2fef86754aca048830ce8250f9` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_C_7_W.EPL` +`0x220` |
| AMD-CHNK | 392720 | `d9721294dbf40e27c9f1a08361453f31931be6cc1d1b29db985572ead642a98d` | 9 | `data.cpk::battle/boss/e215.bin::BED/e215_END.BED` +`0x72160` |
| AMD-CHNK | 136800 | `dd6c784651f4a571fb89868c393d96bd279f34a53a0fb9def181fc2cdd9850bd` | 4 | `data.cpk::battle/cutin/syoukan/SYKN_E109.EPL` +`0x2E0` |
| AMD-CHNK | 324576 | `e18a9fb20636d8e0bbc325c845ca29df60c18f4d8021d64fc2a76f1e4a728d35` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_B_2_W.EPL` +`0x220` |
| AMD-CHNK | 324672 | `e25b64a0b2ffc85be44ceb9fcec6a39247d295f3425e628873ea0ec9ee0eaa47` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_C_2_W.EPL` +`0x220` |
| AMD-CHNK | 134208 | `e289efc0460da8c88834338c2a0c8f0f23bbbd2c1d4b9a4e6a7924c9cbe8be8e` | 2 | `data.cpk::battle/cutin/syoudaku/BOKO_SD_2_W.EPL` +`0x2650` |
| AMD-CHNK | 366624 | `e3d386c9e808f4b085ab68ea60a474f561e324126192ffe40b8b13d281b5cf15` | 2 | `data.cpk::battle/cutin/boko/koma3/3KOMA_B_8_S.EPL` +`0x220` |
| AMD-CHNK | 333824 | `e436f9a7384d7d329376ca2d81f575041d4b656a9d9f3d2b884e19c94b94b94e` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_C_2_S.EPL` +`0x220` |
| AMD-CHNK | 341888 | `e4bcd979bfcfbfee3d931efbac3996178b16362b0e1e351e93ed32f7048656db` | 1 | `data.cpk::battle/bike/B118_S.BED` +`0x18C0` |
| AMD-CHNK | 363040 | `e731f965784afe43d2073d9d6965390483a16c96478187467fa6245cd273b23c` | 1 | `data.cpk::battle/bike/B119_S.BED` +`0x1A5B0` |
| AMD-CHNK | 221600 | `e8bc7b3c8a880872dcc18880c9fbbbc6537b021eab7f1a93541b623215109ad5` | 2 | `data.cpk::battle/bike/B11A_S.BED` +`0x3C460` |
| AMD-CHNK | 75712 | `ea1b65f5117dcc4dfb686fbab1e2a5b22fd583689560d68debaf6ba1a208aa8b` | 2 | `data.cpk::battle/cutin/boko/koma4/4KOMA_A_1_S.EPL` +`0x2E0` |
| AMD-CHNK | 340160 | `eb05a6796273ed2a8e86a3bfdfb71ce311b5ad2f574a0876a5b97927eccaa0a5` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_B_2_S.EPL` +`0x220` |
| AMD-CHNK | 388288 | `edc1d357e1136e8ca85f7237a3c0b9ba2dfe48e46aaa708a1ca5af34427449a4` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_C_7_S.EPL` +`0x220` |
| AMD-CHNK | 44544 | `ef468dd43156fdb6e0c7c1d2d6af2ee0612962368767b0477b54bcff749b084d` | 1 | `data.cpk::battle/cutin/boko/koma2/2KOMA_A_1_W.EPL` +`0x5E0D0` |
| AMD-CHNK | 658464 | `ef7460f9696bd994abd2d07e67075a3361763d4d06fd72abf87d8b3691ee08b5` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_C_4_W.EPL` +`0x220` |
| AMD-CHNK | 336256 | `f01394d200a8c229198d1587998ba1e003478e4aa1f3959cb0d99d179c4548f3` | 1 | `data.cpk::battle/cutin/boko/koma2/2KOMA_B_2_W.EPL` +`0x220` |
| AMD-CHNK | 382400 | `f1eadbb0d44e71f6ad9e95219a058af5c02aa6f2bea6629812328ed1643cadd4` | 1 | `data.cpk::battle/cutin/boko/koma2/2KOMA_B_6_S.EPL` +`0x220` |
| AMD-CHNK | 108320 | `f2689f978ceb494e1117c04e5f844299b6cd2a377e4ae17bb22cf05ef85ba607` | 4 | `data.cpk::battle/cutin/syoukan/SYKN_3.EPL` +`0x2E0` |
| AMD-CHNK | 371872 | `f45968072c2f5453d11dc01db93e05d334dad46c00a95170f239b5418e1bcd7d` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_B_3_S.EPL` +`0x220` |
| AMD-CHNK | 382208 | `f5eb5cfb75f2285044f4ed8e082682b9d8e272c9ed9118415233f66a57488e51` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_C_6_S.EPL` +`0x220` |
| AMD-CHNK | 391808 | `f9654da4300efed02fb8bb21ce3a55c1c22647442fc77ca2aea48d3bf3984ecf` | 1 | `data.cpk::battle/cutin/boko/koma2/2KOMA_B_3_S.EPL` +`0x220` |
| AMD-CHNK | 1600 | `fbaf8460164e642e808456f2b52af2733fc64837e81d32501496170920666ae2` | 4 | `data_k.cpk::battle/battle_pack.bin::btleff.bin::ATC_J_J.BED` +`0x950` |
| AMD-CHNK | 340096 | `fcb574a4eadb1cea674d36112198e68c6278feaefa9e9f8382c48e9e5a5e1a23` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_C_2_S.EPL` +`0x220` |
| AMD-CHNK | 326968 | `ff49baf876bd0fe2ebdc9b1fa008d3e4f1d0dbbd97857f8f525336263e481696` | 1 | `data.cpk::battle/assist/ASSIST_P001.BED` +`0x820` |
| GMO | 68700 | `00fed4a8378c51bdbb19fc4250b1d041f3f184a1b27f3d231a93c1d254610d41` | 1 | `data.cpk::cutin/C5_21.EPL` +`0xA590` |
| GMO | 23928 | `010613dbb70a3f514a32e54d82ee6800331a751f1bf785a7e06f26c8f212634c` | 1 | `data.cpk::battle/cutin/syoukan/SYKN_63.EPL` +`0x460` |
| GMO | 2944 | `0106685c598508eb218363fcde639fb92bc79823e9c4c44e75c86cd862e89793` | 8 | `data.cpk::skill/11C.pac::11C.BED` +`0x21D0` |
| GMO | 6132 | `029804892ea6cd7efbee71948d1b71dd9965d6d9746ab38b908f176bab0b368c` | 1 | `data.cpk::skill/04B.pac::04B.BED` +`0x20690` |
| GMO | 12328 | `02b6569adb9201461216a9d9dca42836787abaa3c9426ccb8639940ac153442b` | 12 | `data.cpk::skill/11D.pac::11D.BED` +`0x21460` |
| GMO | 295872 | `02d2fab3143bb257b61c7cc0ac08f1ce8faf86c9eba7eed1bf22e200bcd0c6b4` | 1 | `data.cpk::battle/cutin/tokusyu/TOKUS_6_S.EPL` +`0x220` |
| GMO | 20548 | `0367a9b5c52140d9dc3de253effe0600c4e496ddfc263ada320150464734ce68` | 1 | `data.cpk::skill/16D.pac::16D.BED` +`0x63E0` |
| GMO | 20260 | `0389a9d367469f83d3bd8a56a157f2e88d2432d416913e7c4760d4f45536a31c` | 1 | `data.cpk::skill/0AD.pac::0AD.BED` +`0x2B00` |
| GMO | 7412 | `039011e7f8e32841a1ec49ca9280d65d0c12bef233dcc87b06ae38f331b3a7c0` | 24 | `data.cpk::skill/100.pac::100.BED` +`0x2C850` |
| GMO | 13092 | `0703fe34754509bc589d4f518dae7ee1e3bfefa7532790b8e8c185096755ab67` | 1 | `data.cpk::skill/069.pac::069.BED` +`0x424C0` |
| GMO | 22532 | `0705cf79342d5c46c9c8414813ce490f60b45da24251cfa1880e87a1a935412c` | 1 | `data.cpk::skill/04B.pac::04B.BED` +`0xAAE0` |
| GMO | 5356 | `073ad4389a4439dd27f1403841a4d14ae2b8687a3539ddd7a7322015ab52a22b` | 1 | `data.cpk::skill/040.pac::040.BED` +`0x23910` |
| GMO | 4364 | `075b730bfb1136800a7a953608b08000ed8ea1e2f1156747f20951336a0e97cb` | 9 | `data.cpk::battle/boss/e200.bin::BED/e200_END.BED` +`0x6490` |
| GMO | 3052 | `09cedd84d9ba890b0bed72bf502bcefac534afa4c9522d2c491179fbc5e4474a` | 9 | `data.cpk::battle/boss/e203.bin::BED/e203_END.BED` +`0x63A0` |
| GMO | 14820 | `0adfc6f3f36d928ab5fb8ba2d63f2ff6b1fc1773ba4011e585aa423ff61680d9` | 1 | `data.cpk::skill/080.pac::080.BED` +`0x1B00` |
| GMO | 25912 | `0d85cae63aa580e7d812fe955e3e76581e75c47b46567bc815318a26f7f8bafa` | 1 | `data.cpk::skill/08C.pac::08C.BED` +`0x1070` |
| GMO | 3788 | `0dafd7940e24032c72d0866e35ffc3aacc7309ea808abbac5834c4b97fc5e9a1` | 9 | `data.cpk::battle/boss/e208.bin::BED/e208_ENCOUNT.BED` +`0xB10` |
| GMO | 2352 | `0e894467d60e18e056f63d4a6dd09f3021d7dd0c29e0775cf1b1ee28bb4dfc2d` | 4 | `data.cpk::battle/union/U15D_A.BED` +`0x1890` |
| GMO | 11304 | `0eca7492b05240e8df23079ccd6cf58714b7ee51664fd967bcaf4f66f2f5f0bc` | 2 | `data.cpk::skill/179.pac::179.BED` +`0xA60` |
| GMO | 864 | `0ee1a1aea18c9012669f502bccd919ce0d6d516e0198f8669070bb26b139d8ab` | 2 | `data.cpk::skill/05C.pac::05C.BED` +`0x8BD0` |
| GMO | 18936 | `0f55e7e0a95d5557457d8e2735e77ae8eb2728758d22752b4c7b67257675048a` | 18 | `data.cpk::event_data/community.bin::CARD_EF04.EPL` +`0x20E00` |
| GMO | 25148 | `0f710e2ed7c29f5d28f0dd814316714529644ae5c4c69bba57fc6e01841d1622` | 1 | `data.cpk::skill/0A0.pac::0A0.BED` +`0x6B50` |
| GMO | 3244 | `0f8e3654795021c0a16e19e2f55d3d6ac460519a30a195926a02160e75353e35` | 9 | `data.cpk::battle/boss/e204.bin::BED/e204_ENCOUNT.BED` +`0x16B0` |
| GMO | 22336 | `108b8429bd128787c87c560f38b3351d62e664f182f2f4c58583f958b98c17ab` | 2 | `data.cpk::battle/cutin/boko/koma3/3KOMA_A_1.EPL` +`0x990` |
| GMO | 102312 | `109b8db4e79e435d8fca4bc38eb4194fb01af4e3861d90a0e7009601664c30cc` | 1 | `data.cpk::battle/assist/ASSIST_EF.BED` +`0x1B30` |
| GMO | 1204 | `121875baafa298579a669605b6da7015ae5c52aa32cf2e72958957f3ddd6fe21` | 100 | `data.cpk::skill/100.pac::100.BED` +`0x2BCD0` |
| GMO | 37668 | `121adeaf4331bff0d07296d0283fe4a3feef0c24e2efcaf7785a310b83b2808d` | 3 | `data.cpk::battle/bike/B119_S.BED` +`0x7D40` |
| GMO | 18516 | `12c0e4ad40c9ded96ed709c38b460b65c371fc314c7ff5dba1bee45f719555a4` | 1 | `data.cpk::skill/04D.pac::04D.BED` +`0xB3C0` |
| GMO | 17872 | `12e3efb1646faf9ccd466fd7ac986e334c5a34bd9b4027afae99a48e804e9b33` | 1 | `data.cpk::skill/181.pac::181.BED` +`0x503F0` |
| GMO | 3728 | `1318455de1e8b689aa72ea796edd39e7ba22ddb8840e724c691e4bad32e7fd7f` | 1 | `data.cpk::skill/17C.pac::17C.BED` +`0x162B0` |
| GMO | 1648 | `135fb6fc4a272f4d2fd9cfb642366191a1599ff35701c14aea65f1363eaa26c4` | 31 | `data.cpk::battle/battle_pack.bin::btleff.bin::R_BLOCK.BED` +`0x1D60` |
| GMO | 6628 | `1382bfe635a380dd251cd382a22a9c9730dde718a18b04f44f4e8556ec484fdd` | 3 | `data.cpk::skill/03D.pac::03D.BED` +`0xA780` |
| GMO | 4888 | `14de34acdc8d2c2cc51eb561346388239bd4d0243e98c3e4785d9f4b4dab8202` | 9 | `data.cpk::battle/boss/e206.bin::BED/e206_H172.BED` +`0x9F0` |
| GMO | 6120 | `14ef2d458850028cdd5ffc26211f1a6c25ca4122f2802531a00784aef337b0a7` | 1 | `data.cpk::skill/08B.pac::08B.BED` +`0x3D30` |
| GMO | 3180 | `154ba31e24bd0627d689a94bb2f12d8b8566f6b4bda768eca873c92f7863d0c2` | 9 | `data.cpk::battle/boss/e208.bin::BED/e208_END.BED` +`0x6E60` |
| GMO | 6108 | `165f44fff3b956d445607bda2848546812746f96f32d907e6214351948f12d09` | 2 | `data.cpk::skill/042.pac::042.BED` +`0x1BC0` |
| GMO | 3436 | `16d8d639fd885f6013699b80f63a29f4a5eda0a12a12db61a66fbeb8d32b5035` | 1 | `data.cpk::skill/180.pac::180.BED` +`0x20F0` |
| GMO | 8756 | `17cfc4a28c6b699864a4d8c8a5a73ae95df58e0e983dfeafd908c17b5b0bbcc5` | 2 | `data.cpk::battle/assist/ANALYZE.BED` +`0x13140` |
| GMO | 35404 | `18ef817138c77c5ae804897f8987e3a474127df087852dfc7dfc30f4b6405606` | 1 | `data.cpk::skill/037.pac::037.BED` +`0x15560` |
| GMO | 6132 | `19b75a7dec5dacd66fdb929f64e61b883b04c348459c7a271322f4ce2367a2ce` | 1 | `data.cpk::skill/04B.pac::04B.BED` +`0x1D550` |
| GMO | 1728 | `19d0bd3eca166eed8c22636d9b17a90eec791c3dc03dceacd7821de3aa2fa6ff` | 1 | `data.cpk::skill/116.pac::116.BED` +`0x1C370` |
| GMO | 192252 | `1a580c92876861cfa61f906d24b803e4bdfc8a494e830f9e3cc0bc809ace741d` | 1 | `data.cpk::skill/0F8.pac::0F8.BED` +`0x1800` |
| GMO | 3744 | `1a7d0bc3cf3bfaa054ae9468e735a0721440f5f37092f81476643ce1b43fb9c3` | 4 | `data.cpk::battle/bike/B11B_S.BED` +`0x3570` |
| GMO | 3744 | `1aeed946e8fd277c4e9cbc64a8a8fe716f43300ca6b59c0ffc141b588d134262` | 3 | `data.cpk::battle/bike/B118_W.BED` +`0xD890` |
| GMO | 2112 | `1b4245d385646f6307a23898394dbad1ec32ad4f5cb955b23584b86ff6fb36c7` | 9 | `data.cpk::battle/boss/e215.bin::BED/e215_END.BED` +`0xD32A0` |
| GMO | 752 | `1cef0f314a05cf9d2231a87b5f3642f5d62e5bde0f640ade43b1073ee9776d48` | 1 | `data.cpk::skill/15D.pac::15D.BED` +`0xB9D0` |
| GMO | 8852 | `1e31babe952421ed4ed04391aabba3230391dfb676bc5b50d1b524dfb439a306` | 16 | `data.cpk::battle/cutin/syoudaku/BOKO_SD_2_S.EPL` +`0x2E0` |
| GMO | 6132 | `1ec47ac758099ae5a1c17f047d9f8472828b1511ea0e241572d219fba5db1e71` | 1 | `data.cpk::skill/04B.pac::04B.BED` +`0x21F30` |
| GMO | 11736 | `1ee2cad8d4b8330c01f2fec1e6b34ed186890f4b5d634dd69d19c0e3636a2498` | 1 | `data.cpk::skill/103.pac::103.BED` +`0x5C50` |
| GMO | 212792 | `1f36f069debefb0961507da9da35ee0d9ef73545e25e94f1a61222584a4a5f39` | 3 | `data_c.cpk::battle/union/U15E_A.BED` +`0xC2D0` |
| GMO | 15064 | `20ebffad9038c6007d01a7de2a138919b67d43cc2753324b62f20d1ca2ec1ef7` | 1 | `data.cpk::skill/15D.pac::15D.BED` +`0xBF90` |
| GMO | 2384 | `22377c93d78271b0d9d935aca34265e83312bfd44adf68c195cd3f3bdb188d2f` | 2 | `data.cpk::battle/bike/B118_W.BED` +`0x1E60` |
| GMO | 4612 | `2251e32e17824c943861081d4692f6cb7e1ef783fc1c9e2a7aca9880f23f2d1a` | 24 | `data.cpk::skill/102.pac::102.BED` +`0x31C80` |
| GMO | 295872 | `237cd17a3f511601c8c86dc09c6d2aa251fa99ac2fbe9dcafdfaf84cbf7337fa` | 1 | `data.cpk::battle/cutin/tokusyu/TOKUS_7_S.EPL` +`0x220` |
| GMO | 20048 | `2388ec13b59cfcb4d55586f78ffb8810fd8dcbc5020913fce0af468c302fc352` | 1 | `data.cpk::skill/103.pac::103.BED` +`0xEA20` |
| GMO | 4912 | `243df97a299ae693b5fa060a3bcc738304136dbcdd78b4075b872182028dee6f` | 1 | `data.cpk::skill/16B.pac::16B.BED` +`0x16D0` |
| GMO | 23936 | `2599144b9af90fe76cfb2be52a2b2babd86de0f5c7ed3a812bf8ce00a75d9430` | 1 | `data.cpk::battle/cutin/syoukan/SYKN_10B.EPL` +`0x460` |
| GMO | 1300 | `25b6b82532bee5d3ef82eacefaed923bfd47e40ac4472facf167d6a71d4c567d` | 2 | `data.cpk::battle/cutin/boko/koma3/3KOMA_A_1.EPL` +`0x3A0` |
| GMO | 8852 | `2624cc238e037663360d73d03d54777c852408067f28dd8ddc8d6d27bf073a88` | 12 | `data.cpk::battle/cutin/syoudaku/TUGK_SD_2_S.EPL` +`0x2E0` |
| GMO | 2612 | `266c164cb969685b654b5271f9441de8fbbcaddb4be432f3c0f1ca1fdfbeeb24` | 1 | `data.cpk::skill/105.pac::105.BED` +`0xDCA0` |
| GMO | 6824 | `269ad91821a157710589347ee93136c0ed52811b279273aaa9dbdac1535b846a` | 3 | `data.cpk::skill/021.pac::021.BED` +`0x4860` |
| GMO | 3988 | `26b759cb70be2a7e414b4e9842d0b1c72ad70f82909a7b316d12e4941d519f13` | 8 | `data.cpk::skill/023.pac::023.BED` +`0x4480` |
| GMO | 2732 | `26f309eae55b8e47f9c1bc8223331838379f70cd7f733072af735e47039d8a76` | 9 | `data.cpk::battle/boss/e205.bin::BED/e205_END.BED` +`0x1160` |
| GMO | 1300 | `276ad079fb3a5297bf3cb8f1460e3d18c1e03aab37adbc729258998346d55e56` | 2 | `data.cpk::battle/cutin/boko/koma4/4KOMA_A_1.EPL` +`0x3A0` |
| GMO | 7096 | `27b314df79bd7da880feecf5e0e92b147a8b02c246ea2a1a3784b89ad5b6467c` | 3 | `data.cpk::skill/03D.pac::03D.BED` +`0xF870` |
| GMO | 68748 | `29006dd6cb6a7d8822ef4eaa938bd1ca2e51a4e1e6b80d99abaa1666171a54a0` | 1 | `data.cpk::cutin/C5_40.EPL` +`0xA590` |
| GMO | 295880 | `2a5437708a77cc5e2ddbc7cbffd96794a6ae8838943e08e707f89f15b50bb710` | 1 | `data.cpk::battle/cutin/tokusyu/TOKUS_4_W.EPL` +`0x220` |
| GMO | 20688 | `2bba12b3622880a5d920326189275eb8ca1c5985eebf8e376eb73a3dd60b4c9c` | 6 | `data.cpk::skill/02E.pac::02E.BED` +`0x18790` |
| GMO | 3212 | `2c5119e9330eb66ca45f59b141a8c7205c01c2645316782e9e2dee3fee00dfd6` | 9 | `data.cpk::battle/boss/e204.bin::BED/e204_ENCOUNT.BED` +`0x990` |
| GMO | 4736 | `2c9d25dac364ba2e13614486b06bc0711b6ae9760c7f49aaa00c2acf50682182` | 9 | `data.cpk::battle/boss/e200.bin::BED/e200_H160.BED` +`0x1110` |
| GMO | 4152 | `2cc13ad5dea8189fce600b2b04b3bf412501deab09f4bafd911c627c400f43b8` | 8 | `data.cpk::skill/023.pac::023.BED` +`0x98A0` |
| GMO | 1196 | `2d654cfdf14b3ab59224a18cb0f5bc08c5c236e37bca926af8569370fbab78d4` | 48 | `data.cpk::skill/100.pac::100.BED` +`0x2F340` |
| GMO | 5580 | `2f7901fc8f586712c16c1c9013e0d586187e4693a40ab9d275b4a05046c51ca1` | 9 | `data.cpk::battle/boss/e214.bin::BED/e214_ENCOUNT.BED` +`0x11F0` |
| GMO | 19308 | `309de052ad5dba6443a8ab57853c469ae63ed6cb6a1b726da3378d7cb6d8d58f` | 18 | `data.cpk::cutin/C5_20.EPL` +`0x3A0` |
| GMO | 6400 | `31ce315b75a2b16a6176cd81a73cf71010e8b0f59090a822f5df5b3f98508330` | 1 | `data.cpk::skill/040.pac::040.BED` +`0x21F70` |
| GMO | 19620 | `32a378efc72649e5c572ba9affe2581e71d1438e687d287c689f1f044bf596e0` | 1 | `data.cpk::skill/03F.pac::03F.BED` +`0x4DE70` |
| GMO | 6460 | `3323a8913ba391df1e7d7c1d083a8f4607390eb9d69009c54af0037b9f595f9f` | 8 | `data.cpk::skill/01B.pac::01B.BED` +`0x2EB0` |
| GMO | 2968 | `336f56b48245e43f97db2016c1ba31d810245ebb213b43a41b3172917d8972b6` | 9 | `data.cpk::battle/cutin/syoukan/SYKN_108.EPL` +`0x9930` |
| GMO | 2712 | `34edc1aa9bb184c84784c9df7f977fd5ff4cc955de132cd4f51dffe11713b4ad` | 1 | `data.cpk::skill/181.pac::181.BED` +`0x4D160` |
| GMO | 4684 | `35bb9cc7fbb572d9e6956740e71bdd5d6c65853352439b4098ead41ac9b47364` | 9 | `data.cpk::battle/boss/e204.bin::BED/e204_END.BED` +`0x8D0` |
| GMO | 3072 | `369474775de6f4176b8578eb6880072ac718891c235c37e739c2471fa426dd17` | 9 | `data.cpk::battle/cutin/syoukan/SYKN_108.EPL` +`0x8C60` |
| GMO | 295880 | `380ad58e4b07e5f4a842174eaf20eaa7faa7ffc579a9995cc11b547da20b56f3` | 1 | `data.cpk::battle/cutin/tokusyu/TOKUS_4_S.EPL` +`0x220` |
| GMO | 4888 | `38983fe35cf41144fea6e00758c81a80c7b54e99cfdefc7d7669ab737d7e0893` | 10 | `data.cpk::skill/06D.pac::06D.BED` +`0x1D2D0` |
| GMO | 2052 | `39d558f0b8ea1074e31d66e139470e53ace1d879606346b9d58739abf76fd604` | 16 | `data.cpk::battle/battle_pack.bin::btleff.bin::ATC_D_J.BED` +`0x950` |
| GMO | 12412 | `39e2295493d908be7fca9b2b88d5ed137fb942bbcdfdd450fe7d17bb218f8ac8` | 9 | `data.cpk::init.bin::event/icon.bin::_ICON3.EPL` +`0x1F0` |
| GMO | 8048 | `3a5d7755215f5857884ca802a0bc77799ff99c65069c7303e6095479b56acf26` | 1 | `data.cpk::skill/019.pac::019.BED` +`0x6690` |
| GMO | 68428 | `3cb578350648c9a75f5f7d1f1b33136630d59557fe35f7c5dcd568244b3fa790` | 2 | `data.cpk::cutin/C8_20.EPL` +`0xA590` |
| GMO | 68700 | `3d24d5f0d2c2162406af655cad77ea81a8834b08f22940c0f1ba43357f845f7b` | 1 | `data.cpk::cutin/C5_22.EPL` +`0xA590` |
| GMO | 2856 | `3da54ce8137cc4581d5a5d1f17b6b62e28482747ebd880302b6aa26ae67aff36` | 22 | `data.cpk::battle/battle_pack.bin::btleff.bin::DOWN_Y.BED` +`0x8A0` |
| GMO | 1472 | `3e6632e4b137fdd61a137767a59ec632d2293345784414485e3602b407b49702` | 27 | `data.cpk::battle/boss/e215.bin::BED/e215_END.BED` +`0xD2040` |
| GMO | 13092 | `3eb8e0a75da37654d6a6a274b893be0a9ebd8a64cdc98927e690ff5a44be2d7d` | 2 | `data.cpk::skill/040.pac::040.BED` +`0x1B7D0` |
| GMO | 7500 | `40e6bc5b807b7ba9b25510b7298f7dcb3860ad4b8191455620b0bdbd38d78168` | 1 | `data.cpk::skill/040.pac::040.BED` +`0x2F5F0` |
| GMO | 70084 | `41960e9abf158dbacc49bc39b2668e3f84fc612bfe9ac57de3d1cad7134a1cd1` | 1 | `data.cpk::skill/043.pac::043.BED` +`0x1B1F0` |
| GMO | 21068 | `4304d0fa02ee129eefb17f5ff35b3bb8d1372e9ced09e8296ad7522e161fd9ab` | 1 | `data.cpk::skill/088.pac::088.BED` +`0x11E0` |
| GMO | 13092 | `43dd05bb4c14ed45bd23d1f197a8d13ccedf5a8cd1c87be1ff459ad90b9cdff2` | 2 | `data.cpk::skill/03F.pac::03F.BED` +`0x428C0` |
| GMO | 4172 | `459b261c1ad50454a5b50a8d174c02bbbc699355aceb9eb42a8504dcef1cd0a6` | 9 | `data.cpk::battle/boss/e201.bin::BED/e201_END.BED` +`0x10A0` |
| GMO | 8848 | `46a150a7af78634dbd07c642fcba2764bc92679bf2b1cbe7cc38ffd8cf1b713e` | 1 | `data.cpk::skill/0F7.pac::0F7.BED` +`0x24180` |
| GMO | 6724 | `46f08c939405680aa64c311d7b796462bf75c8212e5d4278f5aa9da8c778b830` | 9 | `data.cpk::battle/boss/e216.bin::BED/e216_ENCOUNT.BED` +`0xCF0` |
| GMO | 6596 | `4727b7576edb69cf5961ec84ad955cb77f0fb5ffa8d22f26ee1741f3749e4952` | 3 | `data.cpk::skill/03D.pac::03D.BED` +`0x8D10` |
| GMO | 19232 | `47b743cf2235846cbcd9f62bd3a3d385b675209b67d09eefc07310506bccc311` | 1 | `data.cpk::skill/097.pac::097.BED` +`0x6180` |
| GMO | 104056 | `47dc9cc1b0d6dabcd9a363e379e4b462431b5bc1f48b6abbc15e1f009ef8f462` | 4 | `data.cpk::battle/cutin/syoukan/SYKN_8.EPL` +`0x2E0` |
| GMO | 5120 | `481939034a9b9fd3813fbea417fd25870f271588466f47ddf7dbdd95299dc1ff` | 9 | `data.cpk::battle/boss/e20B.bin::BED/e20B_END.BED` +`0x2010` |
| GMO | 6132 | `48480c155d0eb1639bec169f39ef44823fe582b0cbdc249137ac3ac5b301d6a7` | 1 | `data.cpk::skill/04B.pac::04B.BED` +`0x1BCB0` |
| GMO | 23936 | `48ee5297ebbe3ba78c70fe8c4b7776a0449e7e364aaf5c19c97aa3675e73ff59` | 1 | `data.cpk::battle/cutin/syoukan/SYKN_12A.EPL` +`0x460` |
| GMO | 10448 | `49a8e551a9b2bb8f7f5cad0057787bfa515bf61c9cea15cf91d2fb31f13cbe6c` | 3 | `data.cpk::battle/cutin/syoukan/SYKN_63.EPL` +`0x62B0` |
| GMO | 15880 | `4b0818a9d77cfbc9553387b0ca16223d1014fb0b63789ddb8c98cc9903437f61` | 2 | `data.cpk::skill/04E.pac::04E.BED` +`0x1CCF0` |
| GMO | 4596 | `4b8525d90fc8fbe2349a15374388e2c27223ee2be18b6a0a362b310520907eab` | 9 | `data.cpk::event/fishing.bin::FLOAT_01.EPL` +`0xA30` |
| GMO | 8128 | `4cb568adca382038bea708c29c430f431020943b653cc9b5b7413daa676b2a80` | 1 | `data.cpk::skill/182.pac::182.BED` +`0x25820` |
| GMO | 37708 | `4ddc977cc43c1060c93188b9dd9817844a4456437890245d4e1c3df0e9478241` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_A_1.EPL` +`0x6560` |
| GMO | 23936 | `4e42378892a6b5dbd20f8096b1761e90ff4b7363fa349ff93831ed73e7c6a981` | 1 | `data.cpk::battle/cutin/syoukan/SYKN_126.EPL` +`0x460` |
| GMO | 212880 | `4fd7b2b23962f02873ae5ee5ba969447161c24aad258fbc0f8befacac022b0d7` | 3 | `data_c.cpk::battle/union/U15F_A.BED` +`0x9830` |
| GMO | 23928 | `50097a29c542e6ca55eca7d8c8af245222374b11bca794146614c869939336a7` | 1 | `data.cpk::battle/cutin/syoukan/SYKN_A.EPL` +`0x460` |
| GMO | 14688 | `5062b45b98e7f0e03ff3abb2b1c364c4ee6202ef49bd4c29c93b7e9f3bf449ff` | 1 | `data.cpk::skill/0AA.pac::0AA.BED` +`0xA130` |
| GMO | 68428 | `5225748890690cf9b51779c9cc05ea5cd63be6ede831cfb0a06550e39b50491d` | 2 | `data.cpk::cutin/C8_22.EPL` +`0xA590` |
| GMO | 6948 | `524fc8356fa7d007f32c40bafd91209accaaea9bb1748c9e2d2518748bd4923b` | 9 | `data.cpk::battle/boss/e216.bin::BED/e216_END.BED` +`0xB70` |
| GMO | 4012 | `536a02cc5099fd5c564ae6af740e212dbdce2295886dc949a7050f4370c6ca18` | 9 | `data.cpk::battle/boss/e20A.bin::BED/e20A_END.BED` +`0x3280` |
| GMO | 12872 | `53feee57a69047529b8ce028590eafec2fd319dda3f0759e36a72566be2d3d1f` | 2 | `data.cpk::skill/04E.pac::04E.BED` +`0x15990` |
| GMO | 10992 | `5459e349765be895bcf92a9b328b820b0fe8b06b7d4f4537597996d42b3c4cdc` | 1 | `data.cpk::skill/042.pac::042.BED` +`0x77C0` |
| GMO | 3948 | `5468351b69ec489984056f4258eb7fcef005d4a3fa8f0bbc2086dd7119c0444e` | 18 | `data.cpk::battle/boss/e201.bin::BED/e201_ENCOUNT.BED` +`0x990` |
| GMO | 2588 | `546efbf8c956a6f2db2075fed36c20929600d7b335e828541d879215b3c96642` | 9 | `data.cpk::battle/boss/e203.bin::BED/e203_END.BED` +`0x7020` |
| GMO | 1212 | `55691e0f6c3ac448ed597996cbcb6a141c6677834a74bfd88ae4b9485786a642` | 4 | `data.cpk::skill/101.pac::101.BED` +`0x2BF50` |
| GMO | 1736 | `55f913159a4ba1a5cb3b688534b07b7bc578f476897742e380dbb49dedec614b` | 24 | `data.cpk::skill/172.pac::172.BED` +`0x19CC0` |
| GMO | 7596 | `56808f0a98b4e852adbc1a2469750de47df767219020ed9add3619d5cb71c915` | 2 | `data.cpk::skill/01C.pac::01C.BED` +`0x154E0` |
| GMO | 3020 | `57f6cf649b70f2f7addd3821aab23c8669fa832223c5a15c1131bab4f9015087` | 22 | `data.cpk::battle/battle_pack.bin::btleff.bin::BUH_CRT.BED` +`0xA10` |
| GMO | 1888 | `57f8f5ba99bdbd0f4043a7a3ee06a0944acab203185fa3a2677340adf2cbd9f1` | 1 | `data.cpk::battle/effect/CRT_HP.BED` +`0x7E0` |
| GMO | 18708 | `59e99ef8e64ca15e127c6523505cea9881e6a2b5c1b1b23aeaa8bc2a6f8ec485` | 1 | `data.cpk::skill/040.pac::040.BED` +`0x26330` |
| GMO | 15880 | `59fbfd46d8068abfc3145740d9a159f82c0467ce4b502c9e7578d91671d0c1b2` | 1 | `data.cpk::skill/04E.pac::04E.BED` +`0x11A00` |
| GMO | 69140 | `5ae2662a4fef7d635ae2aaa68a36a038091b653d0dd894ffd09edc4c896d1fa9` | 1 | `data.cpk::skill/16E.pac::16E.BED` +`0x28D0` |
| GMO | 2796 | `5b4cf10f649947816a218e38d02c2f345ff85d3b24f8b19b92928f4e82f57d66` | 9 | `data.cpk::battle/boss/e208.bin::BED/e208_END.BED` +`0x1160` |
| GMO | 210356 | `5ba309ac5f6b81c9ef686a3ada4bc9b0e172507b2618862fe96bf3fad89dfb29` | 3 | `data_c.cpk::battle/union/U15D_A.BED` +`0x4950` |
| GMO | 3692 | `5ccd9b3fc617af329ca0a215521dc6d55f340edbeb35d805dcf1b0db27b4e716` | 9 | `data.cpk::battle/boss/e205.bin::BED/e205_ENCOUNT.BED` +`0x990` |
| GMO | 3264 | `5cf9b001662ab215c5add2bff68a4503e14f193a19be0c8be64602455d6e1f9c` | 4 | `data.cpk::skill/11C.pac::11C.BED` +`0xED40` |
| GMO | 68756 | `5d011b77bc2f72a68d2e3f916713db553c9a8cf61f2d9df8fadd2b0413afdf93` | 1 | `data.cpk::skill/096.pac::096.BED` +`0xFA0` |
| GMO | 40616 | `5d5e6f2694ac04c7f2faef628fbab77fbc0108ce3023fea63062afde1970e803` | 10 | `data.cpk::skill/06D.pac::06D.BED` +`0x132D0` |
| GMO | 37820 | `5ed6f54e64a9385e71870846ab9b1f08bfd28d069d78f5b731c07ad63a399462` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_A_1.EPL` +`0x61A0` |
| GMO | 68748 | `5f3fe009dd3a6b59c9e068224679d2ee8184d6f6dda7230e4cb1e214e6614fb7` | 1 | `data.cpk::cutin/C5_41.EPL` +`0xA590` |
| GMO | 5180 | `5f6024c03ab27f17819cc5c2ad5a6bd83f664a8386cab94d64f46c85d6a1e52f` | 9 | `data.cpk::battle/boss/e20A.bin::BED/e20A_ENCOUNT.BED` +`0x810` |
| GMO | 3920 | `60bede96182f7fa17f46795b555cf50e5f571bc26126fee5f6c4dce83d2dbb6a` | 1 | `data.cpk::skill/0F7.pac::0F7.BED` +`0x22FB0` |
| GMO | 23292 | `613f287d16d18b3b7a7c3999355bf2e7dcd5487e588de0f184e15e520a9d068e` | 2 | `data.cpk::battle/cutin/boko/koma4/4KOMA_A_1.EPL` +`0x990` |
| GMO | 1936 | `619efb627824c7928691f10dc0ea46868b8d446a297f82e9077714d283394ae7` | 4 | `data.cpk::skill/172.pac::172.BED` +`0x1A570` |
| GMO | 3420 | `6204fff06f8148668c34c91f6779660f71efb86e2f97288a3c1b2587eebd67bd` | 2 | `data.cpk::skill/164.pac::164.BED` +`0x1420` |
| GMO | 7596 | `637e82beb884f065e06253c20199629e47666a2ce0cfc3965f70365e54e6af04` | 2 | `data.cpk::skill/01C.pac::01C.BED` +`0x13690` |
| GMO | 4660 | `63b8731674096c7d184693b197f0b422e2fba5c840b3510610a11f40c59c1190` | 8 | `data.cpk::skill/023.pac::023.BED` +`0x54C0` |
| GMO | 27700 | `63da2274cc8a2d0699a60ca4fecdff08c4a5fcebeabb6f3068f38e2189514928` | 1 | `data.cpk::skill/169.pac::169.BED` +`0x17540` |
| GMO | 34064 | `63db5b087ea3ce139808f434806a0de62f2c06b235a433ed0dc8b71f0f6d9c20` | 2 | `data.cpk::skill/01C.pac::01C.BED` +`0x2B30` |
| GMO | 3436 | `644f8b96e3e9cb437611f8cc00ee73ccfeb20e42ebf331abc99498aea11797ce` | 18 | `data.cpk::battle/boss/e201.bin::BED/e201_ENCOUNT.BED` +`0x19F0` |
| GMO | 38548 | `658bd1d7d5f75a8929a87c5dc11ed5306a95007f72038098e8126455339ab63c` | 4 | `data.cpk::skill/01B.pac::01B.BED` +`0x4890` |
| GMO | 19812 | `65c0e13fca36efe157609cf669b865889aea7e55bdd5eb45deab26545c04933b` | 1 | `data.cpk::skill/03F.pac::03F.BED` +`0x49060` |
| GMO | 39776 | `67723287e49377f5308131cd2b5b1b8131e155c0a14134f1e0dd515280b032ca` | 1 | `data.cpk::skill/019.pac::019.BED` +`0xA170` |
| GMO | 6120 | `68b072506bc30374ba041a0962466bd67ba2daa5a35836db3df02eab3f075e46` | 1 | `data.cpk::skill/04B.pac::04B.BED` +`0x18AE0` |
| GMO | 3420 | `68f9b52d8741a3e44809a86b4afb512597bcd649ce04e91a814d806bbe96de90` | 22 | `data.cpk::battle/battle_pack.bin::btleff.bin::HATUDO_H.BED` +`0xB10` |
| GMO | 18408 | `69716f136ef96259f46997de6b88a58156693537132dbd5fb3f3fc15716aef87` | 1 | `data.cpk::skill/0AD.pac::0AD.BED` +`0x7F80` |
| GMO | 34308 | `69aee070ebdc2e62cd8f34f6fe67d5df98faf304ce386609869e917d5bb19933` | 1 | `data.cpk::skill/04B.pac::04B.BED` +`0x10390` |
| GMO | 15340 | `6a77617f577e3646b9314852a037298681555ae90b7d93fb1ff0cb09dae6026d` | 1 | `data.cpk::skill/0F9.pac::0F9.BED` +`0x3610` |
| GMO | 37820 | `6b5ee8b9076a2a8755ec7ff5b5c8286b9e706addcfef3d7c3d707ecd4de3c4fc` | 1 | `data.cpk::battle/cutin/boko/koma3/3KOMA_A_99.EPL` +`0x61A0` |
| GMO | 19232 | `6bff2eba31be75b7af200a9db01fe56234d3272439b160896820b9d85dce980f` | 1 | `data.cpk::skill/097.pac::097.BED` +`0x1500` |
| GMO | 18516 | `6cb19dc2260922a4104af0ed661bb0e3aa9954786f14df221192d2dcfc2b29f4` | 1 | `data.cpk::skill/040.pac::040.BED` +`0x2ACF0` |
| GMO | 111940 | `6e45ed2acd9d69c1d7564b97802b480d48d98caa4f06174c967de4aaeed632f4` | 1 | `data.cpk::skill/0F7.pac::0F7.BED` +`0x44F0` |
| GMO | 21592 | `6e4e734acabbc351faa4150ce1b62b562db58a4ef9b716efc82aa307c49f225a` | 1 | `data.cpk::skill/08A.pac::08A.BED` +`0x10150` |
| GMO | 3632 | `6fa75249bef053957d22fb8370d1af080fa7c3b9994e9efe9ade9696fa4cb52d` | 3 | `data.cpk::battle/bike/B11B_S.BED` +`0x26B0` |
| GMO | 14588 | `70ba59447bb328d69150f1f85c0c8bf88515828b9ba20f92844c0f68b16c6027` | 1 | `data.cpk::skill/04D.pac::04D.BED` +`0xFCC0` |
| GMO | 816 | `714be542cd1ac2c5b97a3459589e074860969b2d58aeaedef180a6583a80b154` | 2 | `data.cpk::skill/143.pac::143.BED` +`0x1D80` |
| GMO | 184812 | `720323ad98407db77cb441c53b6c66421a2c51af5fe59c3fda57d4031045a30d` | 1 | `data.cpk::skill/15E.pac::15E.BED` +`0xE070` |
| GMO | 18592 | `72cfd20df762d93c75c24244865e5a0349e5bf085de6599a9426f7cc8b6d5696` | 6 | `data.cpk::skill/0D3.pac::0D3.BED` +`0x1360` |
| GMO | 69624 | `7306739b3fe64f1c67af1911e0b50d51f89b7e693abbd9605e482f81e10960bc` | 1 | `data.cpk::skill/09C.pac::09C.BED` +`0x130C0` |
| GMO | 1016 | `730c8cc7bb68ec482ee6849b05d1457f724dc0cf665d46f317ba4db1c84869b8` | 9 | `data.cpk::battle/boss/e200.bin::BED/e200_END.BED` +`0x7880` |
| GMO | 1244 | `737c0918e275996c81d8175165b612121779c8ef33618bf3861357c8a630a19d` | 1 | `data.cpk::skill/04E.pac::04E.BED` +`0x2CD10` |
| GMO | 38380 | `7387d15875d84565a58f2f7e550c0676fed758b37d6ff990c3b91a63182c6dbd` | 1 | `data.cpk::skill/091.pac::091.BED` +`0x11E0` |
| GMO | 210356 | `73ed38474279b7115d97946383a135c6912704c4c68abb49c31b1f0278063cbf` | 1 | `data.cpk::battle/union/U15D_A.BED` +`0x4950` |
| GMO | 2240 | `7444fdd23d534e86d4390d73009d73bd531a0265f49083f23052abf00997edf6` | 9 | `data.cpk::battle/boss/e215.bin::BED/e215_END.BED` +`0x75A0` |
| GMO | 295872 | `744fd514a29d27bd2f4d0dad5a0e48b567748549999335b46895cdce006ed130` | 1 | `data.cpk::battle/cutin/tokusyu/TOKUS_7_W.EPL` +`0x220` |
| GMO | 6676 | `74718d70414e765e85ef31c9d7a966aff1288eb493643a02d0384abd016e1faa` | 8 | `data.cpk::skill/023.pac::023.BED` +`0x7DE0` |
| GMO | 19392 | `75fbb82f1da51537c9eca8fe71f9f0a888c9d33d26a022ddecdc6fb1ba8d326e` | 1 | `data.cpk::skill/04B.pac::04B.BED` +`0x5E80` |
| GMO | 3688 | `7645d6a53b2275ad4329d860775ae75a0e15a924bb28938fe5b8e66a9417f11d` | 1 | `data.cpk::skill/0FA.pac::0FA.BED` +`0x3DC0` |
| GMO | 180132 | `766951cab609f5e34ef3f3431a901f7b01e98843f29bbeded14c62e9b1f362da` | 1 | `data.cpk::skill/171.pac::171.BED` +`0x2B40` |
| GMO | 19756 | `7a130064abd2fc6a443321d049357b01e8281ea5489c94d33d699af827487c42` | 10 | `data.cpk::skill/06D.pac::06D.BED` +`0x8AA0` |
| GMO | 2112 | `7a1fc7824770371f37af001079e577d2209407996660aadeabca1148ff89b4ea` | 1 | `data.cpk::skill/11A.pac::11A.BED` +`0xBD0` |
| GMO | 240224 | `7aaf41240d8530744398e470ec53f7a6710edc4961429fa1d5709200a7880871` | 1 | `data.cpk::battle/assist/ASSIST_RISE_S.BED` +`0x4260` |
| GMO | 3056 | `7bae55a18a50592b6ca87151c9a96a51c8047753f0ab854a48622be76ebbc4a1` | 9 | `data.cpk::battle/boss/e215.bin::BED/e215_END.BED` +`0x4030` |
| GMO | 100056 | `7d4cd486e07a0d9feaf19b3da998b5722267a80bbd8284c52f41eacf2d6b1e3a` | 1 | `data.cpk::skill/11D.pac::11D.BED` +`0x8C30` |
| GMO | 6444 | `7ddec3b0213745f3683190b7b4bad25edad2eff40046036cfaff66c0303ef7dd` | 1 | `data.cpk::skill/01A.pac::01A.BED` +`0x19C0` |
| GMO | 3548 | `7e13b44bdb8337accf9844066571e7aaccba4a34fa514ad81ffa55cea2f870c0` | 22 | `data.cpk::battle/battle_pack.bin::btleff.bin::ZIO_CRT.BED` +`0x1020` |
| GMO | 18368 | `7e7c5f2177593b6e30aeb8b553d7bde982282e1fb12a6c66612f3db574d98404` | 1 | `data.cpk::skill/00C.pac::00C.BED` +`0x24D0` |
| GMO | 11368 | `7e8fdda9b45414a02b84364b725f059990feda6cd59793f18f2b21d2d1f76ca8` | 15 | `data.cpk::skill/0D1.pac::0D1.BED` +`0x2E10` |
| GMO | 5400 | `7fd076bd24aa818f16a00f1bf0a5c3c4716386f3a99554cf1f823d7b45a02e89` | 1 | `data.cpk::skill/171.pac::171.BED` +`0x1280` |
| GMO | 20924 | `80190330fe498a3d894c82c04f7f7501c90c1395ea770707efe1e96ec59dcbd3` | 1 | `data.cpk::skill/092.pac::092.BED` +`0x15D50` |
| GMO | 6128 | `80d31231a528adbebee1a9a914c010309bee7bd7b28e23541b1f7523e6fcffb3` | 1 | `data.cpk::battle/assist/ASSIST_EF.BED` +`0x1C0C0` |
| GMO | 3184 | `815784524201c5061694ce007ede1f2b0468858a68e62d6d11050c5ab0f7f1ee` | 1 | `data.cpk::battle/bike/B118_W.BED` +`0x6A630` |
| GMO | 21144 | `82166312f2be7b64caef408e5a23670c43336383d3ab7c0ec711c1481388aedf` | 10 | `data.cpk::skill/06D.pac::06D.BED` +`0xD870` |
| GMO | 6132 | `8466e214b289f4752a2c72b3e3c0da4581501924da7d528b9c175605ae7a0be0` | 1 | `data.cpk::skill/04B.pac::04B.BED` +`0x1A410` |
| GMO | 2144 | `84757d013dd7b176877b51a9d086255148453dd311ba5ab2485880ccb3c547b0` | 1 | `data.cpk::battle/union/U15E_B.BED` +`0x4630` |
| GMO | 5408 | `848942a861967f9f4e4e18725d0ae8b276ee141750cd5b686b4a3fa54a2b6f8d` | 3 | `data.cpk::skill/16E.pac::16E.BED` +`0x1310` |
| GMO | 11476 | `85e85c13c8d493a4853e0fabf5b3583e4f798a001dd1b05750b434b376fcfbb4` | 1 | `data.cpk::skill/04D.pac::04D.BED` +`0x85A0` |
| GMO | 1600 | `87f9060c9f4c8225fb90e5408eb59f554c60bdc15f5ef9863dd59c07c51ec400` | 27 | `data.cpk::battle/boss/e206.bin::BED/e206_OFF.BED` +`0xA70` |
| GMO | 1420 | `889f6c6caf14c201a82dd946d0fb64eadd9c7bf89953543af322de2292ce5b8e` | 16 | `data.cpk::skill/101.pac::101.BED` +`0x30E30` |
| GMO | 68428 | `8bad409106aef862001e4fd4eaaecbf58acf64b832b477bfc1da10c8131044c8` | 2 | `data.cpk::cutin/C8_23.EPL` +`0xA590` |
| GMO | 25480 | `8c57bc002f9d02c87d5f858734c345d8d2a5af51b02a170556ac00212ab1760f` | 22 | `data.cpk::battle/battle_pack.bin::btleff.bin::R_BLOCK.BED` +`0x25C0` |
| GMO | 23928 | `8d063e94a1dadd020285370f1dbedca924d7b9189187087268780c110f8ef70b` | 1 | `data.cpk::battle/cutin/syoukan/SYKN_9.EPL` +`0x460` |
| GMO | 1888 | `8d10fd9d60507bf1db48ef6969c10033953418d2b0d48a2ae95f558c3e96c7f9` | 1 | `data.cpk::skill/0FA.pac::0FA.BED` +`0x2E60` |
| GMO | 37668 | `8d8a733a141a402de33b8dd01c690450ebd06fff7e0d12b8fdf9ae9e33b101d0` | 3 | `data.cpk::battle/bike/B119_S.BED` +`0x111E0` |
| GMO | 17872 | `8d9feec889607ea46c64efdedd86babdaa0943c8f3c5c3de552cf7ea1ae97c64` | 1 | `data.cpk::skill/18B.pac::18B.BED` +`0x4D960` |
| GMO | 69640 | `8e543bbf4e13a91669eacafb802a82be153b52bb03525e620fd610c96f2f4b1d` | 1 | `data.cpk::skill/09C.pac::09C.BED` +`0x1130` |
| GMO | 11736 | `8e8ad772b920f3e94754e2a8e00b3fe8de6bd07412dfde5b08437874ccaffdb8` | 1 | `data.cpk::skill/103.pac::103.BED` +`0x8BA0` |
| GMO | 3508 | `8f7d22330d21d3a6fce535764a5e8d3f711682ef1502d4cbecb75ea49eae9c5f` | 9 | `data.cpk::battle/boss/e206.bin::BED/e206_ENCOUNT.BED` +`0x990` |
| GMO | 45324 | `9027b3d6985f9a01619c56d3eb76b227ffa53565d5a376cc055a41424790c738` | 1 | `data.cpk::skill/15F.pac::15F.BED` +`0x7710` |
| GMO | 20916 | `903b72cfaeee7c22afea5e01680243325867718c2845f12515c6a8cf4011c416` | 1 | `data.cpk::skill/081.pac::081.BED` +`0x10D30` |
| GMO | 1728 | `9094b4b3fe7afd0909052f21ec7c99947d6b0c42e8e3ccb302cae1d5ceded98b` | 1 | `data.cpk::skill/117.pac::117.BED` +`0x16140` |
| GMO | 93136 | `9155bcbaa6f637f23701c36be3284165e8b8421ed85739b519769ca63f52f527` | 22 | `data.cpk::battle/battle_pack.bin::btleff.bin::R_BLOCK.BED` +`0x89F0` |
| GMO | 22028 | `9206e976fade20059f3dd000a5467ab66eaec707e6ac03356c39ea1fceb31dfa` | 1 | `data.cpk::battle/cutin/boko/koma2/2KOMA_A_1.EPL` +`0x990` |
| GMO | 4784 | `935780c4a2a493db7ed3eada531429737c0c5104edbfd7776f48e0f586510d75` | 4 | `data.cpk::battle/assist/ASSIST_EF_S.BED` +`0x32D0` |
| GMO | 3328 | `984bb71e4f42dd5dcd5902a3b8cd1b73270435415830e86fd8d61c14ce184a9f` | 1 | `data.cpk::battle/assist/ASSIST_EF.BED` +`0xC60` |
| GMO | 15388 | `989ffa09cda2a0fc95f3e0681d3950360edd07d45c398a695169457f84f8f7c2` | 2 | `data.cpk::battle/assist/ANALYZE.BED` +`0xF480` |
| GMO | 4620 | `98c01b31af60d1dbfd1daefd978d1006ec1f0350c03f8446e03ae35da13e136e` | 9 | `data.cpk::battle/boss/e20A.bin::BED/e20A_END.BED` +`0x1FE0` |
| GMO | 3388 | `98f351a6ad87350726a0e39ab22bc32360c1f775f8e65eec3bb674ed22a9550f` | 9 | `data.cpk::battle/boss/e20B.bin::BED/e20B_END1.BED` +`0xE30` |
| GMO | 68700 | `99d52e58b90720f70d6b2fb051fdbff4055219fe281af929ae8cf1823d0f8c24` | 1 | `data.cpk::cutin/C5_33.EPL` +`0xA590` |
| GMO | 36736 | `9a13efd91b06012f2395c0b85dfffc426bc96bc4661095183e9fb09747c12c2f` | 1 | `data.cpk::skill/08D.pac::08D.BED` +`0x167E0` |
| GMO | 32432 | `9a66c4920adabcbb5a39030c3085761c735525237c52e14f215bc70fd60239bd` | 9 | `data.cpk::init.bin::event/icon.bin::_ICON4.EPL` +`0x1F0` |
| GMO | 18408 | `9a9be397fc16cf790431787fca11bfd831ab749c883d6db7129d093b25238fac` | 2 | `data.cpk::skill/0AB.pac::0AB.BED` +`0xA390` |
| GMO | 2672 | `9acc9eef8dbc3d7f82beaaafac300c44bbac4d3d5a379ac78bcf97f530bf7650` | 3 | `data.cpk::battle/bike/B11A_S.BED` +`0xBF0` |
| GMO | 37708 | `9b39feae57c4a470f38dcf76644ab63a8415212605f28840f3699aac5befae22` | 1 | `data.cpk::battle/cutin/boko/koma4/4KOMA_A_99.EPL` +`0x6560` |
| GMO | 68700 | `9bc725b541006b9c4f524f72fd3750343466fd816057fe6d4b898429310b30c4` | 1 | `data.cpk::cutin/C5_32.EPL` +`0xA590` |
| GMO | 8112 | `9c3641c279b518dfa04e029f5cf9e9bfc44856f19530eacf9b88b1517e2c1b5b` | 3 | `data.cpk::skill/0E2.pac::0E2.BED` +`0x1F60` |
| GMO | 6548 | `9cd810ba46f12d843a15bc03cb6852b89600aeaa65e0bc987e6ac0677fab179b` | 9 | `data.cpk::event/fishing.bin::FLOAT_WV.EPL` +`0x2B0` |
| GMO | 6364 | `9ce0d53ce1768190d7638a36c649cfb94a638900c3dcf775a8e62abe08ae1201` | 6 | `data.cpk::skill/02E.pac::02E.BED` +`0xEB60` |
| GMO | 15880 | `9db2a301c0a3db85d2d6b17ca5b648c5c19e00255ef878ae7fca299d503dcda6` | 2 | `data.cpk::skill/04E.pac::04E.BED` +`0x18D60` |
| GMO | 4236 | `9e7c2558ede690cc1807f54fcb340a0c9c201b72ad32d0fb5de17be7a628684b` | 81 | `data.cpk::battle/boss/e209.bin::BED/e209_ENCOUNT.BED` +`0xA50` |
| GMO | 3768 | `9ea76fbbcb569b0febbcd930ad9c6635624ad59c3b0fec37ef8eda95eaeb3719` | 22 | `data.cpk::battle/battle_pack.bin::btleff.bin::ATC_J.BED` +`0xA60` |
| GMO | 3320 | `9f72b9fe3695e195525bf0a9ae734b13241c9403148c754a8f4fb48a7d2e72b9` | 1 | `data.cpk::skill/119.pac::119.BED` +`0xAA0` |
| GMO | 21068 | `a0be2809b411ffe59fe215738f33f7937bcf188d112db49ee2db44002170b91f` | 1 | `data.cpk::skill/17C.pac::17C.BED` +`0x9840` |
| GMO | 3196 | `a124f5d5c3037305fe1c2098b9e9ebc7d89ecad1a1775a790731e50c54bedadc` | 9 | `data.cpk::battle/boss/e203.bin::BED/e203_END.BED` +`0x7AD0` |
| GMO | 19044 | `a1ff07ae8ee406330f83c03861695d675afbadd731c1b987a0975702c0a77338` | 1 | `data.cpk::skill/117.pac::117.BED` +`0x6D00` |
| GMO | 10452 | `a23ada8a2934279e5673f1057a8f6c19383b11b0d1f7b69c3fef4e5c2d48c598` | 6 | `data.cpk::battle/cutin/syoukan/SYKN_108.EPL` +`0x62B0` |
| GMO | 6572 | `a332ce6c41d26e01fc0c840592f85fc5aa34efe78fa8812dcc0429fb56543cd9` | 22 | `data.cpk::battle/battle_pack.bin::btleff.bin::KYUSYU.BED` +`0xE90` |
| GMO | 7860 | `a46068d1fc123f99874ab33eb1d74e62c35031d560bb3c6ae1b0005914e37acb` | 1 | `data.cpk::skill/079.pac::079.BED` +`0x1310` |
| GMO | 1244 | `a4ca34fedc369cdfdab386360f83c7fbc31f2798e626dc433f9fe51823073b03` | 1 | `data.cpk::skill/04E.pac::04E.BED` +`0x2D290` |
| GMO | 11384 | `a4e699c2bfeb9ae960f3cba2b03b25a7204834e2ca58e069c45ea7925ec421ae` | 5 | `data.cpk::skill/038.pac::038.BED` +`0x17690` |
| GMO | 2176 | `a4e7725187b08e99e0232ade2c909f0a02f5c8687cdfc640e904ad3a9afc45e3` | 3 | `data.cpk::battle/bike/B117_S.BED` +`0xA20` |
| GMO | 4012 | `a4f1af00273cd1f3e2a99947d453fbe10938f9a908f6a19ca0c3c8b955304e2e` | 1 | `data.cpk::skill/17F.pac::17F.BED` +`0x1A60` |
| GMO | 1228 | `a67cac292a22b3f4a64dc770fa8d439f0c2a8bd87b143bc981688dbf16b0128e` | 16 | `data.cpk::battle/battle_pack.bin::btleff.bin::ATC_J_J.BED` +`0x950` |
| GMO | 14112 | `a7fd148401d0ff61cd94a14590f4979d52106b614fc4d3d09d63c1102a733ca7` | 8 | `data.cpk::skill/023.pac::023.BED` +`0xAE80` |
| GMO | 10300 | `a8a5f9318779f61044bbbe33907bb7ce3e60c7ce0da5380dcf95f951749be049` | 4 | `data.cpk::skill/0FA.pac::0FA.BED` +`0x5470` |
| GMO | 68700 | `aa2a4aa21328179c0646934b82256644989aa7ce7d47496ba7cb78c0a10fbe64` | 1 | `data.cpk::cutin/C5_31.EPL` +`0xA590` |
| GMO | 2864 | `aac4ce9df0fde70d6363078dc164f36572050359a6370dfd88ebda7a6a1baf68` | 1 | `data.cpk::skill/17C.pac::17C.BED` +`0x171D0` |
| GMO | 864 | `aae255f2d07fbde6cda071f8c8c865202bec6f74107e19a92b9831c44ef39c2a` | 2 | `data.cpk::skill/0D9.pac::0D9.BED` +`0x1D30` |
| GMO | 68700 | `ab9dfac779b6f049ab23fd827d76d4808730e3324bcfb7001d54ca5d6c509df7` | 1 | `data.cpk::cutin/C5_23.EPL` +`0xA590` |
| GMO | 4748 | `ac806f232fc83089d40d66582c41ee41a2ffd0c40550b8144f80bdda39537f3a` | 9 | `data.cpk::battle/boss/e20C.bin::BED/e20C_ENCOUNT.BED` +`0xA50` |
| GMO | 3860 | `acb3d15fbc72425eb78e68e18f106d2ad5ffff6d0e86faf186c380088cccd43b` | 9 | `data.cpk::battle/boss/e200.bin::BED/e200_ENCOUNT.BED` +`0x1590` |
| GMO | 5100 | `ad15f5b49dee5c1732c6d9f998909de581618588171f266c394dcbd1006a46de` | 1 | `data.cpk::skill/040.pac::040.BED` +`0x24EA0` |
| GMO | 1336 | `ad6c27abee27d211c3a2c3e134f35901b13fab8960eb9071e9a1871ce21dd2e1` | 1 | `data.cpk::system/tool/effect/sample/O_HLP_UT0.EP` +`0x60` |
| GMO | 23428 | `add698601e60c515348c88a9ac17bfd6da777ed2bb2ab9444ac2f587755f477b` | 1 | `data.cpk::skill/038.pac::038.BED` +`0x1FF80` |
| GMO | 35348 | `adf77f1f2680de346436bacb724b6aa4c18e96807da422127b7d0ba07dc91164` | 8 | `data_k.cpk::battle/cutin/syoukan/SYKN_1.EPL` +`0x1A9B0` |
| GMO | 158172 | `ae8d76b54bfc929084591fbe3deb042e1046c5567e61bde9e2f2b0f3a98fdf78` | 1 | `data.cpk::skill/16F.pac::16F.BED` +`0x28D0` |
| GMO | 12184 | `af900c374aec89ffb74c8d6a4421ceda2e9f818820e30aa9f45425e1fd67cb46` | 1 | `data.cpk::skill/043.pac::043.BED` +`0x4160` |
| GMO | 3088 | `b148d42731e2f0c519fada09ad2537607f3bfeeeba2365980a8ae914426ec5bf` | 9 | `data.cpk::battle/boss/e205.bin::BED/e205_KAIJYO.BED` +`0x810` |
| GMO | 70900 | `b1bceb4f8751ca95129bb191ca49239ad240d34bb47e25096ead49269e145047` | 1 | `data.cpk::skill/15F.pac::15F.BED` +`0x128C0` |
| GMO | 3212 | `b1d48c8adc85adf136017004009e4d7d3e5db9cb88362f19faabf02003a40256` | 9 | `data.cpk::battle/boss/e205.bin::BED/e205_END.BED` +`0x6E20` |
| GMO | 7032 | `b39db1645bdcd9b115f8592225afed707b1224ef4c33e7ecc8b4d5b8242f17e7` | 3 | `data.cpk::skill/03D.pac::03D.BED` +`0x114D0` |
| GMO | 4716 | `b3cfc84e6f79ffded4400b6da15445c3ab5e92c07c10318e942ed61c9a60bf4f` | 9 | `data.cpk::battle/boss/e206.bin::BED/e206_END.BED` +`0x68A0` |
| GMO | 3500 | `b4235e7e94a587761b0bacddc3565c720321d5b9722bd7b18b2f669a17a1143b` | 9 | `data.cpk::battle/boss/e208.bin::BED/e208_ENCOUNT.BED` +`0x1AD0` |
| GMO | 68700 | `b49593642deb779e2b84fb5f67365298abbd50e7f81908993ac8699d3d5ba09f` | 1 | `data.cpk::cutin/C5_30.EPL` +`0xA590` |
| GMO | 107676 | `b5ff547fbd0c30a061ef57856ee70c0df28dc9de6027754d4a2c9f098f85f4f3` | 1 | `data.cpk::skill/099.pac::099.BED` +`0x29C0` |
| GMO | 5524 | `b7fd3c50f85bb570485908be2dd482b368b5805e173e5654615fabd1f890935f` | 8 | `data.cpk::skill/023.pac::023.BED` +`0x67A0` |
| GMO | 864 | `b84e7262485283cc260cbe9269919920ebcf4a0d79a34a684449dccd24094015` | 6 | `data.cpk::skill/048.pac::048.BED` +`0xF360` |
| GMO | 11832 | `b87490d930c51a7debccee441385a964babae154deb4bf50c819928c904b4f50` | 1 | `data.cpk::skill/042.pac::042.BED` +`0xA3F0` |
| GMO | 295872 | `b915de20831228836b335840a2873886dd521029e108e2dfa9672f3734732b52` | 1 | `data.cpk::battle/cutin/tokusyu/TOKUS_2_W.EPL` +`0x220` |
| GMO | 6472 | `b96ef0b18cce236af0d11452f1be3668eb50c928e3030381ef7ffc85db1433ca` | 8 | `data.cpk::skill/11C.pac::11C.BED` +`0x3F620` |
| GMO | 7072 | `b9c1287aff7d5025708fbc95d259c72bfe34448f298c59b00c5e55d5b8827cd1` | 3 | `data.cpk::skill/03D.pac::03D.BED` +`0xDC30` |
| GMO | 69672 | `bad058848e2196c7070290acdee5e3ee334b51207e20c41aa9b5cb5870b1956d` | 3 | `data.cpk::skill/01A.pac::01A.BED` +`0x4E60` |
| GMO | 6628 | `bae23cf0c4c1b4b72076b8887a67a682386549317ec4614b55f7e53a7db53cca` | 3 | `data.cpk::skill/03D.pac::03D.BED` +`0x7280` |
| GMO | 20048 | `bc4ebe0d350b8350f627dc3090cee5b570b08bfd77dc107ca036c91142ee4bde` | 1 | `data.cpk::skill/103.pac::103.BED` +`0xC90` |
| GMO | 18648 | `bd3cca762d72629f4e524ce79eacd181a70769b475d29ebdf97edc249b106a27` | 1 | `data.cpk::skill/0A1.pac::0A1.BED` +`0x16F0` |
| GMO | 2572 | `bdaf91c4d22834ecd30903335ad7db1092374947ecaffe037b5e720d2077dcf0` | 1 | `data.cpk::skill/105.pac::105.BED` +`0xE740` |
| GMO | 12636 | `be782859cd81eb47d0bfe615afe4a2dd7cd2db3587a519ae2c1901bad4d177c7` | 1 | `data.cpk::skill/09E.pac::09E.BED` +`0x13730` |
| GMO | 1464 | `be9f226ad9aced08c48cdb1c7403c93a61d99a30ec580852b0107d9400bbadfd` | 1 | `data.cpk::battle/bike/B117_W.BED` +`0x65E50` |
| GMO | 157484 | `bfc83ac29212aa541cc7ebdfef110b84038a241d46a7a27662e38957b7f46f27` | 1 | `data.cpk::skill/170.pac::170.BED` +`0x28D0` |
| GMO | 4412 | `bfcfc53a354586740bc5da39e025eb40c0d50dda3d81dd4fcbadac79dd86e1a8` | 9 | `data.cpk::battle/boss/e202.bin::BED/e202_END.BED` +`0x6250` |
| GMO | 20612 | `bfffaf5a82faf98f986b05156602fceb61761f17bf610094fb90b47b7a8f6f41` | 1 | `data.cpk::skill/085.pac::085.BED` +`0x11E0` |
| GMO | 18604 | `c19a8c990f969eba8954cadb3fa9307f2b02a452fc9b3ecbf888899537bff192` | 1 | `data.cpk::skill/07E.pac::07E.BED` +`0x20F0` |
| GMO | 3616 | `c31e66b92fc9131691c6f03e80d31e802a0f9dc40e46420981cfaa3b637fde27` | 4 | `data.cpk::battle/assist/ANALYZE.BED` +`0x154C0` |
| GMO | 6516 | `c75934310d7f2d08a9ae6c975caa2fbf2898d01518c8deb16b506d80a0c11275` | 3 | `data.cpk::skill/03D.pac::03D.BED` +`0xC210` |
| GMO | 3308 | `c7c6dec72b9317dae41618717dc3faebea7ee60b1e134bf7b83b9b617e6a27d7` | 9 | `data.cpk::battle/boss/e203.bin::BED/e203_ENCOUNT.BED` +`0x8D0` |
| GMO | 848 | `c901620d37548a69156f8bb489fb4f1264d24809821f57422e2b6b0eaa87be47` | 1 | `data.cpk::skill/0D9.pac::0D9.BED` +`0x1890` |
| GMO | 1728 | `c9a4f87fc7c0ecbe61ef5b27d43a08912bb6ab08af7ec19442bd7726dbcfe42f` | 9 | `data.cpk::battle/boss/e215.bin::BED/e215_END.BED` +`0x4CB0` |
| GMO | 5332 | `ca5de03fb102b5a0281243f4dff896d8d6ded2f1fbeed31e63dc7a43b4b74055` | 1 | `data.cpk::skill/185.pac::185.BED` +`0x175D0` |
| GMO | 7892 | `caafbeef91b8d4c53b4a1507b164c3d064a28841e138ca1038268817fc1f1efc` | 1 | `data.cpk::skill/090.pac::090.BED` +`0x15F0` |
| GMO | 1112 | `cda37fd4ea6c12155fedabb2724f0a7dc4a48103414e28cfca291bfa76be84f0` | 8 | `data.cpk::skill/023.pac::023.BED` +`0xA980` |
| GMO | 2144 | `cdf24c79f84b0dfe7de3a0db03020fe02179af9c39d30dd417db20a521d6b703` | 1 | `data.cpk::battle/union/U15D_B.BED` +`0x3060` |
| GMO | 278388 | `ceec3532b6f27d556f60ab16e1972b51c64500da43cd13e98f1ec636fc51a052` | 1 | `data.cpk::battle/cutin/syoukan/SYKN_5.EPL` +`0x220` |
| GMO | 8388 | `d0df4de676b52b01f58b957b67ed0529790a6de64543c243f4dc9bb19852f636` | 1 | `data.cpk::skill/15E.pac::15E.BED` +`0x9490` |
| GMO | 23936 | `d0eb20836a4a79951e4c947530682a37efcd2af94d8e71de931fe7e01c7ff17f` | 1 | `data.cpk::battle/cutin/syoukan/SYKN_12B.EPL` +`0x460` |
| GMO | 3244 | `d0ed60e9a59d270624655c67d0b1f6b244bc0dc411212d44f4b990d10fac244f` | 9 | `data.cpk::battle/boss/e203.bin::BED/e203_ENCOUNT.BED` +`0x1650` |
| GMO | 4876 | `d1060f6ec4c58bd23d60a1daae5bfaff1cb5d75687d5471620cd7ef1435994d9` | 9 | `data.cpk::battle/boss/e207.bin::BED/e207_ENCOUNT.BED` +`0x810` |
| GMO | 2240 | `d12e616c59e4d52e5d21fb787894372cae29b231894cebd75c47650a5c26aebb` | 9 | `data.cpk::battle/boss/e209.bin::BED/e209_ENCOUNT.BED` +`0x1B40` |
| GMO | 23936 | `d257453fe07e1bc0e7adf2abcb68da5d27412621feccc426df37c3f8f919eb02` | 1 | `data.cpk::battle/cutin/syoukan/SYKN_10C.EPL` +`0x460` |
| GMO | 12992 | `d276b2143d808d2a07a5d7a456275b282afa077737478ce8f207f617e795a5c4` | 1 | `data.cpk::skill/15D.pac::15D.BED` +`0x84D0` |
| GMO | 295880 | `d2b0df79af715369eed363404739a1de108607992f104cdb9362365285f370f5` | 1 | `data.cpk::battle/cutin/tokusyu/TOKUS_3_S.EPL` +`0x220` |
| GMO | 4096 | `d32420dfc4fa60917065597e444bd58087d34789f7ee97fda937455257817640` | 1 | `data.cpk::skill/186.pac::186.BED` +`0x39B0` |
| GMO | 35476 | `d35683f101fe86143ade9e53270f0149e2c7e53c76648a1fdb282e10487b0d0a` | 1 | `data.cpk::skill/09A.pac::09A.BED` +`0x1670` |
| GMO | 212880 | `d3af717a655f99f6ba952c505d9797a4ec2bd6bb0a9c51b141901957e4e163a6` | 1 | `data.cpk::battle/union/U15F_A.BED` +`0x9830` |
| GMO | 3616 | `d4260f77f229c393908f8589db0535a44c1c09fcc4f2c9a0125fe14f6ecb17fd` | 41 | `data.cpk::battle/assist/ASSIST_RISE_S.BED` +`0x3130` |
| GMO | 34064 | `d4671c8217fe05d6f20b92de2971ace45da3026c5e6e73626c5c6065201dd67d` | 2 | `data.cpk::skill/01C.pac::01C.BED` +`0xB0E0` |
| GMO | 11736 | `d4d08f1629418de8de2c77e4d6c761bc51034174cbaf02ab360517c86e1e4d04` | 1 | `data.cpk::skill/103.pac::103.BED` +`0xBAF0` |
| GMO | 860 | `d4d53d5de9c20151c301b45ea9003979fc3e430dbeebe478c30863ea2b25859f` | 2 | `data.cpk::skill/143.pac::143.BED` +`0x2200` |
| GMO | 41064 | `d57503125c8f1a845b62fba8bdfdb2681ceb9366308a06c18bdb4dd7cb6a9114` | 1 | `data.cpk::skill/0FC.pac::0FC.BED` +`0x3140` |
| GMO | 18004 | `d68bdc3f6d8c01496519a57c2ab6862ca216bbeb6ffecc3686f3daccf89a9919` | 3 | `data.cpk::battle/cutin/boko/koma2/2ICON.EPL` +`0x400` |
| GMO | 3424 | `d69c245693e424e56e9ce108c0aa24de28a012f35007626505859674f20b07ab` | 2 | `data.cpk::battle/assist/ASSIST_EF.BED` +`0x1B2D0` |
| GMO | 7032 | `d83c7987b792d519c31928ba7e337fc7c2ced01ad00a87f102c3478e4bffeded` | 1 | `data.cpk::skill/087.pac::087.BED` +`0x6FD0` |
| GMO | 1300 | `d8d399ad07cf0dd7896620c62143e97217d13befc30b85b709f27709b5358e92` | 2 | `data.cpk::battle/cutin/boko/koma2/2KOMA_A_1.EPL` +`0x3A0` |
| GMO | 231608 | `d90c08690e6c8f44be1c8190b04510d0a8024350aa79f2b813f03d2724b50096` | 1 | `data.cpk::battle/assist/ASSIST_RISE_W.BED` +`0x4320` |
| GMO | 3200 | `d9b8e99f875126b1a048bac55811362574012aaac1dca80f52023d753252691c` | 4 | `data.cpk::battle/union/U15F_A.BED` +`0x8990` |
| GMO | 1288 | `dabff9492796d4b7ff78edfda209f6bced8d6aaee8fe2d1aab159ce8a70692d1` | 34 | `data.cpk::battle/union/U15D_A.BED` +`0xD80` |
| GMO | 81916 | `daf65d99ceaa43aabea79c9d6dd1415f29f9b894c6cad6c2b7fd58139616c9e8` | 1 | `data.cpk::skill/160.pac::160.BED` +`0x2DC0` |
| GMO | 5884 | `db551b56c3a3c6eb6bf700b0ecc9c5e4dc189dcb3ed208c3d3d7837e3cf24f08` | 9 | `data.cpk::battle/boss/e215.bin::BED/e215_ENCOUNT.BED` +`0xD50` |
| GMO | 69472 | `dbedf336609e277e594ac2316e6a0d89d16e3a3f0a429fe1ae40e017e546a8cb` | 1 | `data.cpk::skill/043.pac::043.BED` +`0xA1F0` |
| GMO | 296304 | `dbf96f3bdff4484b4670076c260df6e0e4445e6b0d986bec24d2bfce49b63827` | 1 | `data.cpk::battle/cutin/tokusyu/TOKUS_6_W.EPL` +`0x220` |
| GMO | 20212 | `dc246c84a4ae719382ce6992666a8518be58ff2c23fc2ae0b387001f0b263188` | 3 | `data.cpk::battle/bike/B119_S.BED` +`0xAE0` |
| GMO | 68444 | `dc3f4650ebfe6f26c047305ba2dee8fe2b2595e7c1d2111769223eecfcd5b769` | 2 | `data.cpk::cutin/C8_21.EPL` +`0xA590` |
| GMO | 4908 | `dd06ebfedf298670417559ed2c16db9c4a63fa96eea60ea897e2bea2217bde2a` | 18 | `data.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT2.BED` +`0x810` |
| GMO | 19408 | `dd49cd1dc079f779b3dc037579d76cb23c43464bbd7e8b1ce13b3f41d7d325e9` | 1 | `data.cpk::skill/0A4.pac::0A4.BED` +`0x1C60` |
| GMO | 4780 | `dd73a1dae4f2316d027a735e6c6a1f03561529960bd1f0c9de29730f87ab4c6d` | 9 | `data.cpk::battle/boss/e202.bin::BED/e202_ENCOUNT.BED` +`0x8D0` |
| GMO | 3948 | `dda15f7eccc4c0226ff30f9eea9b7e4e41ca3d63260e01b0f9e77038431effc6` | 9 | `data.cpk::init.bin::event/icon.bin::_ICON22.EPL` +`0x1F0` |
| GMO | 5156 | `dde81893ef1036f96b23c8e17684eda2a7a5dba1f69a0aebce98ad5446aa190c` | 2 | `data.cpk::skill/164.pac::164.BED` +`0x33C0` |
| GMO | 2736 | `de4833717d7e2a3049d07bc5da07d1c1775a9eea99d6393a101349c9c46ea34c` | 9 | `data.cpk::battle/boss/e200.bin::BED/e200_ENCOUNT.BED` +`0x2510` |
| GMO | 133616 | `deb6a361fdcf50f454a5f191640ad9a2f5346c103842b7fe6a09953148f4aad8` | 18 | `data.cpk::event_data/community.bin::CARD_EF04.EPL` +`0x370` |
| GMO | 6912 | `df20655b42ff4348449819d832be2a465eedfe106b14eec7bb650030b03fbbbf` | 2 | `data.cpk::skill/181.pac::181.BED` +`0x4E630` |
| GMO | 23936 | `df468dbd43d7a594e6b20725ac2c8386d20ad173a6bd688ba9e2e7851ca2249b` | 1 | `data.cpk::battle/cutin/syoukan/SYKN_108.EPL` +`0x460` |
| GMO | 18408 | `dffc5f90b491cf06d8777cca5292a6f53faef736eac1d66532bbc3b533e2b102` | 3 | `data.cpk::skill/0AC.pac::0AC.BED` +`0x8270` |
| GMO | 3604 | `e0402fcd069c53f2b924a31b50268b40851b96475c0ec44c861962644bd65183` | 8 | `data.cpk::skill/023.pac::023.BED` +`0x35C0` |
| GMO | 1776 | `e0a759e5355a8cf43c94a1c1e0a030b3ad266764bfc952a2b503c40eb0d139a1` | 22 | `data.cpk::battle/battle_pack.bin::btleff.bin::ATC_CA.BED` +`0xAB0` |
| GMO | 21708 | `e1a774222a5bbc24d046075f8c9cb190d9ce16285b4f8c038b2b3b0363e60b25` | 2 | `data.cpk::skill/143.pac::143.BED` +`0x1FAC0` |
| GMO | 2188 | `e2b2097a3815c9977c3720132f8a42933d1b8b789e88b321ba71a81977038f66` | 1 | `data.cpk::system/tool/effect/sample/O_HLP_CM0.EP` +`0x90` |
| GMO | 21588 | `e603bd6551443e44bd9573bae41d14e078efd1f184f053df01d10a65bb21d884` | 1 | `data.cpk::battle/cutin/boko/koma2/2KOMA_A_1b.EPL` +`0x990` |
| GMO | 5340 | `e6405435a56f0f47c30bdb4c3343baa72956be294873a3f4ff74f27383798971` | 9 | `data.cpk::battle/boss/e20B.bin::BED/e20B_ENCOUNT.BED` +`0x810` |
| GMO | 848 | `e77d6052cf9b02c3f53869c728fce307495cd3f9cfd02dac73c09afcca7451df` | 2 | `data.cpk::skill/05C.pac::05C.BED` +`0x8730` |
| GMO | 68700 | `e7c4091d9f92fff81774bea8ab69d230edd3157443e0111ac7b0e66862720f9a` | 1 | `data.cpk::cutin/C5_20.EPL` +`0xA590` |
| GMO | 295880 | `e8a708d7883ac3c27af986f91ac8a7ea6b5732df6a354b64df907675f14b58f4` | 1 | `data.cpk::battle/cutin/tokusyu/TOKUS_2_S.EPL` +`0x220` |
| GMO | 20368 | `e92e02890aa5cefa7ad324574e1e985adac18d36ce1cd00b5a485513a36c258f` | 2 | `data.cpk::skill/0D9.pac::0D9.BED` +`0x1F5E0` |
| GMO | 5164 | `e99b6c5eb7525a19cb986508a43a748dc6b44b60c9619ee6ec4e854bdf6ebeec` | 22 | `data.cpk::battle/battle_pack.bin::btleff.bin::SPOT.BED` +`0xE10` |
| GMO | 2624 | `eb3b31ced9a0a2ed9b467b65e9a34dfb449beffa230fd9f9fcc6b0ad91f3f80d` | 1 | `data.cpk::battle/bike/B118_W.BED` +`0x69B90` |
| GMO | 35348 | `ed29f2732bbd42f9bf08886230473b2f9cfb32e32185b6974565f1db6da1bd70` | 8 | `data.cpk::battle/cutin/syoukan/SYKN_1.EPL` +`0x1A9B0` |
| GMO | 5520 | `ed2aa5d6974ca4f74e523a35f58d2990488d806c0dfca50618bc9e56c8c52f11` | 10 | `data.cpk::skill/06D.pac::06D.BED` +`0x72F0` |
| GMO | 6680 | `ed8eac2be60ff6424809fab32ae5aff2b63657253dbf81ed23e950d8657a3707` | 4 | `data.cpk::battle/union/U15E_A.BED` +`0x67F0` |
| GMO | 38244 | `edba1061fd2ad1c32816fd4c77e877b36e9651731081d57deb5943da87018c3a` | 2 | `data.cpk::battle/cutin/boko/koma2/2KOMA_A_1.EPL` +`0x6070` |
| GMO | 832 | `ee7aed3c78288ed69939c49dd7ae2f62e73b1f3a1b6fa2e8b2adbf65773963ef` | 1 | `data.cpk::skill/0E0.pac::0E0.BED` +`0x1D80` |
| GMO | 8112 | `eef56c656552806ab76688c5cd98b7eaa4c11c6fa680368a74f8ae6f74ac1236` | 4 | `data.cpk::skill/0EB.pac::0EB.BED` +`0x28540` |
| GMO | 69688 | `f019dc23ea40dfae701be66b22c8ae39e0ad03bd8fcc4d8e15c2c6a2b8167536` | 3 | `data.cpk::skill/01A.pac::01A.BED` +`0x15F30` |
| GMO | 12208 | `f08d40900a7b22248da2c119a95310d170843b583f45092b2af480b3d5c5bcc4` | 1 | `data.cpk::skill/043.pac::043.BED` +`0x71A0` |
| GMO | 295872 | `f17982ee3871ef55fcdc5a51af85026fa82b3947d02176513719fb57d444e02e` | 1 | `data.cpk::battle/cutin/tokusyu/TOKUS_3_W.EPL` +`0x220` |
| GMO | 14100 | `f1ec47d0099ae7c9189eb9cf6858062eaad19f2cf09114495ea498e7a2b162b8` | 1 | `data.cpk::skill/019.pac::019.BED` +`0x13E20` |
| GMO | 15756 | `f2f8cddb7b39e1c2af1ea2970c72ac12adc744caa3e50f9eaaf627f01a7a687e` | 1 | `data.cpk::skill/042.pac::042.BED` +`0x3990` |
| GMO | 3472 | `f3b92e8ce66b2aa7757327ed71aa32f4b5f82f77c56e83eb118b4648e4f28f0e` | 1 | `data.cpk::battle/union/U15F_B.BED` +`0x4750` |
| GMO | 68772 | `f52ceadf0b25ad32c09215ca9db235c9c12f290f2c38087d67c78bbba25002f4` | 1 | `data.cpk::skill/096.pac::096.BED` +`0x11DA0` |
| GMO | 101696 | `f5e8264d3772fc8479a17e66ca8e5d451a0ae82c52dc459604b8917584180c24` | 8 | `data.cpk::skill/11C.pac::11C.BED` +`0x4ED40` |
| GMO | 5308 | `f65b269710eb0194d43b64e7d48e697f01d40697254c724afb76538ae9fae889` | 9 | `data.cpk::battle/boss/e207.bin::BED/e207_END.BED` +`0x6250` |
| GMO | 6132 | `f795c0ce31145e47cf5acf7442d5d1f539c569c7e06489bf40c941217f34c9e5` | 1 | `data.cpk::skill/04B.pac::04B.BED` +`0x1EDF0` |
| GMO | 19292 | `f7d5eab283d249e5379c54cbd8722e8a98c91decffe86552d47d12765bea0014` | 1 | `data.cpk::skill/019.pac::019.BED` +`0x19C0` |
| GMO | 41464 | `f7fdfa0d161926a12a7980801fd3664e32bc47aea24a5f78c5cfb1f19611cf87` | 8 | `data.cpk::skill/11C.pac::11C.BED` +`0x44980` |
| GMO | 143520 | `f8953696eaadcac6b836bc7e3ef53a742468a1a1ae061527edc418f623d94db5` | 4 | `data.cpk::skill/181.pac::181.BED` +`0x54B0` |
| GMO | 4980 | `f8a5143cc987d404c790463c7baaefcf36aa116c5533c7582d04308943d4770d` | 1 | `data.cpk::skill/020.pac::020.BED` +`0x2800` |
| GMO | 12432 | `faa3ed46a89ce0deb44958b3d4323a548175a1fb1614dcb455918f16ef140fd9` | 22 | `data.cpk::battle/battle_pack.bin::btleff.bin::HANSYA.BED` +`0xF50` |
| GMO | 3740 | `fb170f32e9079f599f8c5595bcd27190171cf52997199d5de7f0c56224df9ba4` | 2 | `data.cpk::skill/15E.pac::15E.BED` +`0x1F10` |
| GMO | 21724 | `fb58b81326ad30d7cb2f7ce33c6475d99ea0fe4b4b00731b4846355449f35883` | 18 | `data.cpk::cutin/C5_20.EPL` +`0x4FE0` |
| GMO | 35348 | `fbea9fcca6d4c53479e27baec3d27ac01bbb3d32b004b3f0f52028024b0f61e2` | 16 | `data_c.cpk::battle/cutin/syoukan/SYKN_1.EPL` +`0x1A9B0` |
| GMO | 23428 | `fc224cc82f07641cc46917bbcf3134a10ccbab3bb4d0083bee118c008a7ca0af` | 1 | `data.cpk::skill/038.pac::038.BED` +`0x25BB0` |
| GMO | 4016 | `fe22b28e44e604a8a3c788e99c6d1ca6f38d0e46f5853267debbd2a48f9ba529` | 9 | `data.cpk::battle/boss/e215.bin::BED/e215_END.BED` +`0x5400` |
| GMO | 4304 | `fe25879612c14a67bc04d2595ec847e7c05ef45d2657f0d49de1c566cbe6e775` | 9 | `data.cpk::battle/boss/e215.bin::BED/e215_END.BED` +`0x6440` |
| GMO | 212792 | `fe7c6d0685baef558a797581ab5c4cc5ba520eba90e308ca54f18053f2ecc1df` | 1 | `data.cpk::battle/union/U15E_A.BED` +`0xC2D0` |
| GMO | 295872 | `fe818dddf9b755011b0ed7dcc7b57ac5a557d96472d04429be662ec66f40e89a` | 2 | `data.cpk::battle/cutin/tokusyu/TOKUS_8_S.EPL` +`0x220` |
| GMO | 1920 | `ffbb39e1e1e7fa4b4bb0ecf2a7347c8afd8e2018dc0c858a3f3245205b0a2644` | 4 | `data.cpk::battle/union/U15E_A.BED` +`0xB930` |
| inline-GMO | 1816 | `18e050eba835c91227cdeadaa0b542daceb43529ef36f17f10638551045f54c2` | 2 | `data.cpk::battle/bike/B11A_S.BED` +`0x7269C` |
| inline-GMO | 1264 | `1d245f76b3d75f69e2968327f21155472a042ef12e2a8328f42ace191c733e7e` | 8 | `data.cpk::skill/0FA.pac::0FA.BED` +`0x4CCC` |
| inline-GMO | 1256 | `1f2f367ce351189cd9b9113f3147c16407b8cf1ad0da97c1ce7532adb7b68a33` | 1 | `data.cpk::skill/037.pac::037.BED` +`0xAFEC` |
| inline-GMO | 944 | `500ab5f3abb835460f036b2a6cddd09017d30fe3a5c43376817e8e05b547180b` | 10 | `data.cpk::skill/06D.pac::06D.BED` +`0x12BAC` |
| inline-GMO | 2016 | `526a3e3445b36f76e9194e5d90439c977fe2fa0b01c205b63f36e1eaf251788f` | 4 | `data.cpk::battle/bike/B11B_S.BED` +`0xEDC` |
| inline-GMO | 1616 | `581655d9e6e9d813a71fd3f85a04cb01ccfdc5f31f541aeaaf7e73a3c123eaf2` | 22 | `data.cpk::battle/battle_pack.bin::btleff.bin::ATC_Z.BED` +`0x156C` |
| inline-GMO | 7864 | `62c95707d5783a2a7e71a895b9154bc8dcba78774122940d2a6e5550afb27605` | 16 | `data.cpk::battle/union/U15E_A.BED` +`0x18FC` |
| inline-GMO | 2080 | `67938bab4d9c992158c06078a72f2bf6452341446bc9d930e1b503865689387d` | 2 | `data.cpk::skill/035.pac::035.BED` +`0x90FC` |
| inline-GMO | 2488 | `7131ae2aadb71cd8223e981461eab94a44c16729067f1f6708237a707c691938` | 1 | `data.cpk::skill/019.pac::019.BED` +`0x88FC` |
| inline-GMO | 1512 | `72832fe63c88834882cccd35604f8d43faa1ea337f0ad4d413cf3cff47a3ec67` | 1 | `data.cpk::skill/04A.pac::04A.BED` +`0xA22C` |
| inline-GMO | 1504 | `7613c21865d92c0fbb8820eb8f7f688bf98bc5a0bd3f8a7515935546595b0660` | 9 | `data.cpk::battle/boss/e20B.bin::BED/e20B_END1.BED` +`0x1C0C` |
| inline-GMO | 2776 | `7c23f71871e90c32f563741161ee9e294220c2c80a0da7b7c706c0f5564a270c` | 2 | `data.cpk::skill/181.pac::181.BED` +`0x285EC` |
| inline-GMO | 2528 | `7d732cc430c595a2e55f0a2df5f0356dc594044a113698eb5da50a7e3b59aac5` | 1 | `data.cpk::skill/04A.pac::04A.BED` +`0xAC1C` |
| inline-GMO | 23168 | `823a6c2af95cfc441d015a0932aa55c3e2cc623657313549cfbd6244b3c87cc5` | 1 | `data.cpk::skill/037.pac::037.BED` +`0xD4BC` |
| inline-GMO | 2104 | `856a97c7df318885d682cf6a44fb16d554e6e4f217328f98866ba5e2a1083223` | 3 | `data.cpk::skill/015.pac::015.BED` +`0x2475C` |
| inline-GMO | 1960 | `8657d3215c7be035ec8554fd5d5852b254761183d1bacba081d3ffcad61c7212` | 1 | `data.cpk::skill/060.pac::060.BED` +`0x12DFC` |
| inline-GMO | 1256 | `a0f44940b8e8ec0f6e02748d7e0f4b0c3ac56489e5761706c0b90d17eee9edd9` | 1 | `data.cpk::skill/037.pac::037.BED` +`0x8B1C` |
| inline-GMO | 2148 | `a265842e70ffa6709f553afee8cdca55d868cd1df66d8fb95fc18948a155bf3b` | 2 | `data.cpk::skill/096.pac::096.BED` +`0x2304C` |
| inline-GMO | 2400 | `aa80018c1af0c7bbeb016b3ccaa71d651ed7b821bdf4fd05036209ca8a6bd443` | 1 | `data.cpk::skill/08A.pac::08A.BED` +`0x669C` |
| inline-GMO | 2208 | `bf1d2ee83aee33a23fdf15b792b4f871626533b65d49b28cc83b096def9df1f4` | 4 | `data.cpk::skill/049.pac::049.BED` +`0x11B5C` |
| inline-GMO | 2208 | `bf5e08db1b57e2e39b97a776b83bb87ffddb7cecbd3b3d181d2a45cb04412afa` | 1 | `data.cpk::skill/048.pac::048.BED` +`0xFC0C` |
| inline-GMO | 1608 | `c89fa0f8aad2baa9b662859e21e1c85f189220f86732a5a338c2ea3dbcda6100` | 9 | `data.cpk::event/fishing.bin::FLOAT_01.EPL` +`0x1EC` |
| inline-GMO | 2424 | `de1bd59292db5adeffd4c8b626a7bfedc2ff7860c4bd960a331f47505dd0a6be` | 1 | `data.cpk::skill/160.pac::160.BED` +`0x122C` |
| inline-GMO | 2208 | `f98989297d9990a410b16dac68c05fad545ca1fd463e394974a68f9132ba918b` | 1 | `data.cpk::skill/04A.pac::04A.BED` +`0xFFDC` |
| legacy-F0F0 | 6562 | `0e2104417d6e97adb41eaaaf2756745abfa8067caaa4d13921408ed66d0c4099` | 2 | `data.cpk::system/tool/effect/TV_EFF.EPL` +`0x430` |
| legacy-F0F0 | 20501 | `7029b2ada9bf4cd8fdc5f6a1513939b1ca5ae4cac263ca7a790466ede08435a7` | 4 | `data.cpk::cutin/C5_1.EPL` +`0xDA0` |
| legacy-F0F0 | 71925 | `737e77e063303d7a2f79d817fa73bfb359947a7fbd17f08790611d5f95ce4ac0` | 2 | `data.cpk::cutin/C5_4.EPL` +`0x2E0` |
| legacy-F0F0 | 2349 | `87a6b4d467553d8d80a0874cea7f419ab6ad251f91e19e8307f15b1f948436de` | 2 | `data.cpk::cutin/C7_1.EPL` +`0x3A0` |
| legacy-F0F0 | 2349 | `9616855e95fc3d6e695d2cbf6121053ffc99e20e9d766abe98bbeb95f85ad443` | 2 | `data.cpk::cutin/C5_1.EPL` +`0x3A0` |
| legacy-F0F0 | 37115 | `9f4693f44487b8b649a6846e3d7e24e280607ac2f3a532e398a10e6488f4cad5` | 2 | `data.cpk::cutin/C7_1.EPL` +`0x5E90` |
| legacy-F0F0 | 14710 | `bb46613b7c237be1de87263a4c30e1708e675dc28bd2ae0401926dc9859a15b8` | 9 | `data.cpk::battle/boss/e20C.bin::BED/e20C_END.BED` +`0x11D0` |
| legacy-F0F0 | 206218 | `c232d432b7ed5f067680ae37b179139c9ba63dd9460ca529b045f97c90f4710c` | 40 | `data.cpk::cutin/C2_20.EPL` +`0x220` |
| legacy-F0F0 | 37115 | `c8ec47369c775a860216643e01b08d344a418807598d466c7d8f5437bf744c39` | 2 | `data.cpk::cutin/C5_1.EPL` +`0x5E90` |
| legacy-F0F0 | 21918 | `d046ccd8796638504802edfa570cfd2aaebcb1aac03985e6ffcd37b90274beaf` | 4 | `data.cpk::cutin/C5_4.EPL` +`0x11CB0` |
| legacy-F0F0 | 2562 | `d9b836d690d41a830f9de1c1f3e2533f10892719a56639c50e030e994640a10e` | 9 | `data.cpk::battle/boss/e214.bin::BED/e214_END.BED` +`0x990` |
| legacy-F0F0 | 91189 | `e54ce22dfadc27069c0440a051acd9679987abe30b93e90cdb062f95dc21705a` | 52 | `data.cpk::cutin/C10_1.EPL` +`0x220` |
| legacy-F0F0 | 71925 | `e7e884e38c8080acc26e6cd5f8b43ea37cd03d60fdc26100899973ff0a46db48` | 2 | `data.cpk::cutin/C7_4.EPL` +`0x2E0` |
| tool-PIB0 | 7872 | `69e9d7ebd444215c025610f54722eaccb389b3da13b048eaf9c7f38ee9662d17` | 1 | `data.cpk::system/tool/effect/sample/C_NOMAL0.EP` +`0xF0` |
| tool-PIB0 | 2880 | `b1cdb7ce1147e2bb4e8aa67da13b98f5632a6f4afbe96ce1d2460bc0f34bcb6b` | 1 | `data.cpk::system/tool/effect/sample/O_P2_HT0.EP` +`0xD0` |

Receipt `model-census.json` SHA-256`d91612dea3778405241bc380077e80b9eb9e1ba8c036d3dd730360312db92a53`. The companion [model/helper/drawable contract](m8f-model-helper-drawable.md) specifies EP26/27/32 and source phase/lifetime boundaries. No runtime qualification is claimed.
