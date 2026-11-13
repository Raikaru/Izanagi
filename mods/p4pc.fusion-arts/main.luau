--!strict
-- FUSION ARTS: WEAPON FUSION. In the Velvet Room, a carried Persona is fused into a
-- weapon one fighter owns: the Persona, that weapon (the vessel) and a fee are
-- consumed, and a forged weapon is added. The result depends on the character, the
-- Persona's level band and its arcana; Fool Personas make Wild weapons and fourteen
-- Personas make signature weapons. Forged rows derive from the user's own item table
-- (data/forge.xml); the catalogue below is generated with them.

if p4.settings.get("weapon_fusion") ~= true then
    return
end

local ELEMENTAL_LEVEL = 41        -- elemental signatures from the T3 epoch (Yu at band B4)
local SIGNATURE_RANK = 5          -- Social Link rank that unlocks a fighter's signatures

type Row = {id: number, kind: string, character: number, band: number, key: number, anchor: number,
    value: number, element: number, hit: boolean, name: string}
type Character = {id: number, name: string, arcana: number}

-- Fighters by party ID and the arcana of their Social Link (Yu uses the compendium).
local CHARACTERS: {Character} = {
    {id = 1, name = "YU", arcana = 0}, {id = 2, name = "YOSUKE", arcana = 2}, {id = 3, name = "CHIE", arcana = 8},
    {id = 4, name = "YUKIKO", arcana = 3}, {id = 6, name = "KANJI", arcana = 5}, {id = 7, name = "NAOTO", arcana = 11},
    {id = 8, name = "TEDDIE", arcana = 18},
}

-- BEGIN GENERATED CATALOG (tools/mods/fusion_arts_forge.py)
local CATALOG = [[
2048	generic	1	1	2	3	98	0	0	Magus Blade I
2049	generic	1	1	3	3	31	0	0	Oracle Blade I
2050	generic	1	1	4	3	101	0	0	Queen Blade I
2051	generic	1	1	5	3	107	0	0	King Blade I
2052	generic	1	1	6	3	110	0	0	Abbot Blade I
2053	generic	1	1	7	3	36	0	0	Amour Blade I
2054	generic	1	1	8	3	60	0	0	Rider Blade I
2055	generic	1	1	9	3	26	0	0	Just Blade I
2056	generic	1	1	10	3	39	0	0	Hermit Blade I
2057	generic	1	1	11	3	60	0	0	Wheel Blade I
2058	generic	1	1	12	3	26	0	0	Might Blade I
2059	generic	1	1	13	3	51	0	0	Hanged Blade I
2060	generic	1	1	14	3	42	0	0	Reaper Blade I
2061	generic	1	1	15	3	110	0	0	Temper Blade I
2062	generic	1	1	16	3	45	0	0	Devil Blade I
2063	generic	1	1	17	3	95	0	0	Tower Blade I
2064	generic	1	1	18	3	104	0	0	Star Blade I
2065	generic	1	1	19	3	57	0	0	Moon Blade I
2066	generic	1	1	20	3	26	0	0	Sun Blade I
2067	generic	1	1	21	3	54	0	0	Judge Blade I
2068	generic	1	2	2	10	98	0	0	Magus Blade II
2069	generic	1	2	3	10	31	0	0	Oracle Blade II
2070	generic	1	2	4	10	101	0	0	Queen Blade II
2071	generic	1	2	5	10	107	0	0	King Blade II
2072	generic	1	2	6	10	110	0	0	Abbot Blade II
2073	generic	1	2	7	10	36	0	0	Amour Blade II
2074	generic	1	2	8	10	60	0	0	Rider Blade II
2075	generic	1	2	9	10	26	0	0	Just Blade II
2076	generic	1	2	10	10	39	0	0	Hermit Blade II
2077	generic	1	2	11	10	60	0	0	Wheel Blade II
2078	generic	1	2	12	10	26	0	0	Might Blade II
2079	generic	1	2	13	10	51	0	0	Hanged Blade II
2080	generic	1	2	14	10	42	0	0	Reaper Blade II
2081	generic	1	2	15	10	110	0	0	Temper Blade II
2082	generic	1	2	16	10	45	0	0	Devil Blade II
2083	generic	1	2	17	10	95	0	0	Tower Blade II
2084	generic	1	2	18	10	104	0	0	Star Blade II
2085	generic	1	2	19	10	57	0	0	Moon Blade II
2086	generic	1	2	20	10	26	0	0	Sun Blade II
2087	generic	1	2	21	10	54	0	0	Judge Blade II
2088	generic	1	3	2	17	99	0	0	Magus Blade III
2089	generic	1	3	3	17	33	0	0	Oracle Blade III
2090	generic	1	3	4	17	102	0	0	Queen Blade III
2091	generic	1	3	5	17	108	0	0	King Blade III
2092	generic	1	3	6	17	111	0	0	Abbot Blade III
2093	generic	1	3	7	17	37	0	0	Amour Blade III
2094	generic	1	3	8	17	61	0	0	Rider Blade III
2095	generic	1	3	9	17	28	0	0	Just Blade III
2096	generic	1	3	10	17	40	0	0	Hermit Blade III
2097	generic	1	3	11	17	61	0	0	Wheel Blade III
2098	generic	1	3	12	17	28	0	0	Might Blade III
2099	generic	1	3	13	17	52	0	0	Hanged Blade III
2100	generic	1	3	14	17	43	0	0	Reaper Blade III
2101	generic	1	3	15	17	111	0	0	Temper Blade III
2102	generic	1	3	16	17	46	0	0	Devil Blade III
2103	generic	1	3	17	17	96	0	0	Tower Blade III
2104	generic	1	3	18	17	105	0	0	Star Blade III
2105	generic	1	3	19	17	58	0	0	Moon Blade III
2106	generic	1	3	20	17	28	0	0	Sun Blade III
2107	generic	1	3	21	17	55	0	0	Judge Blade III
2108	generic	1	4	2	18	99	0	0	Magus Blade IV
2109	generic	1	4	3	18	33	0	0	Oracle Blade IV
2110	generic	1	4	4	18	102	0	0	Queen Blade IV
2111	generic	1	4	5	18	108	0	0	King Blade IV
2112	generic	1	4	6	18	111	0	0	Abbot Blade IV
2113	generic	1	4	7	18	37	0	0	Amour Blade IV
2114	generic	1	4	8	18	61	0	0	Rider Blade IV
2115	generic	1	4	9	18	28	0	0	Just Blade IV
2116	generic	1	4	10	18	40	0	0	Hermit Blade IV
2117	generic	1	4	11	18	61	0	0	Wheel Blade IV
2118	generic	1	4	12	18	28	0	0	Might Blade IV
2119	generic	1	4	13	18	52	0	0	Hanged Blade IV
2120	generic	1	4	14	18	43	0	0	Reaper Blade IV
2121	generic	1	4	15	18	111	0	0	Temper Blade IV
2122	generic	1	4	16	18	46	0	0	Devil Blade IV
2123	generic	1	4	17	18	96	0	0	Tower Blade IV
2124	generic	1	4	18	18	105	0	0	Star Blade IV
2125	generic	1	4	19	18	58	0	0	Moon Blade IV
2126	generic	1	4	20	18	28	0	0	Sun Blade IV
2127	generic	1	4	21	18	55	0	0	Judge Blade IV
2128	generic	1	5	2	20	100	0	0	Magus Blade V
2129	generic	1	5	3	20	35	0	0	Oracle Blade V
2130	generic	1	5	4	20	103	0	0	Queen Blade V
2131	generic	1	5	5	20	109	0	0	King Blade V
2132	generic	1	5	6	20	111	0	0	Abbot Blade V
2133	generic	1	5	7	20	38	0	0	Amour Blade V
2134	generic	1	5	8	20	62	0	0	Rider Blade V
2135	generic	1	5	9	20	30	0	0	Just Blade V
2136	generic	1	5	10	20	41	0	0	Hermit Blade V
2137	generic	1	5	11	20	62	0	0	Wheel Blade V
2138	generic	1	5	12	20	30	0	0	Might Blade V
2139	generic	1	5	13	20	53	0	0	Hanged Blade V
2140	generic	1	5	14	20	44	0	0	Reaper Blade V
2141	generic	1	5	15	20	112	0	0	Temper Blade V
2142	generic	1	5	16	20	47	0	0	Devil Blade V
2143	generic	1	5	17	20	97	0	0	Tower Blade V
2144	generic	1	5	18	20	106	0	0	Star Blade V
2145	generic	1	5	19	20	59	0	0	Moon Blade V
2146	generic	1	5	20	20	30	0	0	Sun Blade V
2147	generic	1	5	21	20	56	0	0	Judge Blade V
2148	generic	1	6	2	32	100	0	0	Magus Blade VI
2149	generic	1	6	3	32	35	0	0	Oracle Blade VI
2150	generic	1	6	4	32	103	0	0	Queen Blade VI
2151	generic	1	6	5	32	109	0	0	King Blade VI
2152	generic	1	6	6	32	111	0	0	Abbot Blade VI
2153	generic	1	6	7	32	38	0	0	Amour Blade VI
2154	generic	1	6	8	32	62	0	0	Rider Blade VI
2155	generic	1	6	9	32	30	0	0	Just Blade VI
2156	generic	1	6	10	32	41	0	0	Hermit Blade VI
2157	generic	1	6	11	32	62	0	0	Wheel Blade VI
2158	generic	1	6	12	32	91	0	0	Might Blade VI
2159	generic	1	6	13	32	53	0	0	Hanged Blade VI
2160	generic	1	6	14	32	44	0	0	Reaper Blade VI
2161	generic	1	6	15	32	112	0	0	Temper Blade VI
2162	generic	1	6	16	32	47	0	0	Devil Blade VI
2163	generic	1	6	17	32	97	0	0	Tower Blade VI
2164	generic	1	6	18	32	106	0	0	Star Blade VI
2165	generic	1	6	19	32	59	0	0	Moon Blade VI
2166	generic	1	6	20	32	30	0	0	Sun Blade VI
2167	generic	1	6	21	32	56	0	0	Judge Blade VI
2168	generic	2	1	2	43	98	0	0	Magus Kunai I
2169	generic	2	1	3	43	31	0	0	Oracle Kunai I
2170	generic	2	1	4	43	101	0	0	Queen Kunai I
2171	generic	2	1	5	43	107	0	0	King Kunai I
2172	generic	2	1	6	43	110	0	0	Abbot Kunai I
2173	generic	2	1	7	43	36	0	0	Amour Kunai I
2174	generic	2	1	8	43	60	0	0	Rider Kunai I
2175	generic	2	1	9	43	26	0	0	Just Kunai I
2176	generic	2	1	10	43	39	0	0	Hermit Kunai I
2177	generic	2	1	11	43	60	0	0	Wheel Kunai I
2178	generic	2	1	12	43	26	0	0	Might Kunai I
2179	generic	2	1	13	43	51	0	0	Hanged Kunai I
2180	generic	2	1	14	43	42	0	0	Reaper Kunai I
2181	generic	2	1	15	43	110	0	0	Temper Kunai I
2182	generic	2	1	16	43	45	0	0	Devil Kunai I
2183	generic	2	1	17	43	95	0	0	Tower Kunai I
2184	generic	2	1	18	43	104	0	0	Star Kunai I
2185	generic	2	1	19	43	57	0	0	Moon Kunai I
2186	generic	2	1	20	43	26	0	0	Sun Kunai I
2187	generic	2	1	21	43	54	0	0	Judge Kunai I
2188	generic	2	2	2	48	98	0	0	Magus Kunai II
2189	generic	2	2	3	48	31	0	0	Oracle Kunai II
2190	generic	2	2	4	48	101	0	0	Queen Kunai II
2191	generic	2	2	5	48	107	0	0	King Kunai II
2192	generic	2	2	6	48	110	0	0	Abbot Kunai II
2193	generic	2	2	7	48	36	0	0	Amour Kunai II
2194	generic	2	2	8	48	60	0	0	Rider Kunai II
2195	generic	2	2	9	48	26	0	0	Just Kunai II
2196	generic	2	2	10	48	39	0	0	Hermit Kunai II
2197	generic	2	2	11	48	60	0	0	Wheel Kunai II
2198	generic	2	2	12	48	26	0	0	Might Kunai II
2199	generic	2	2	13	48	51	0	0	Hanged Kunai II
2200	generic	2	2	14	48	42	0	0	Reaper Kunai II
2201	generic	2	2	15	48	110	0	0	Temper Kunai II
2202	generic	2	2	16	48	45	0	0	Devil Kunai II
2203	generic	2	2	17	48	95	0	0	Tower Kunai II
2204	generic	2	2	18	48	104	0	0	Star Kunai II
2205	generic	2	2	19	48	57	0	0	Moon Kunai II
2206	generic	2	2	20	48	26	0	0	Sun Kunai II
2207	generic	2	2	21	48	54	0	0	Judge Kunai II
2208	generic	2	3	2	51	99	0	0	Magus Kunai III
2209	generic	2	3	3	51	33	0	0	Oracle Kunai III
2210	generic	2	3	4	51	102	0	0	Queen Kunai III
2211	generic	2	3	5	51	108	0	0	King Kunai III
2212	generic	2	3	6	51	111	0	0	Abbot Kunai III
2213	generic	2	3	7	51	37	0	0	Amour Kunai III
2214	generic	2	3	8	51	61	0	0	Rider Kunai III
2215	generic	2	3	9	51	28	0	0	Just Kunai III
2216	generic	2	3	10	51	40	0	0	Hermit Kunai III
2217	generic	2	3	11	51	61	0	0	Wheel Kunai III
2218	generic	2	3	12	51	28	0	0	Might Kunai III
2219	generic	2	3	13	51	52	0	0	Hanged Kunai III
2220	generic	2	3	14	51	43	0	0	Reaper Kunai III
2221	generic	2	3	15	51	111	0	0	Temper Kunai III
2222	generic	2	3	16	51	46	0	0	Devil Kunai III
2223	generic	2	3	17	51	96	0	0	Tower Kunai III
2224	generic	2	3	18	51	105	0	0	Star Kunai III
2225	generic	2	3	19	51	58	0	0	Moon Kunai III
2226	generic	2	3	20	51	28	0	0	Sun Kunai III
2227	generic	2	3	21	51	55	0	0	Judge Kunai III
2228	generic	2	4	2	56	99	0	0	Magus Kunai IV
2229	generic	2	4	3	56	33	0	0	Oracle Kunai IV
2230	generic	2	4	4	56	102	0	0	Queen Kunai IV
2231	generic	2	4	5	56	108	0	0	King Kunai IV
2232	generic	2	4	6	56	111	0	0	Abbot Kunai IV
2233	generic	2	4	7	56	37	0	0	Amour Kunai IV
2234	generic	2	4	8	56	61	0	0	Rider Kunai IV
2235	generic	2	4	9	56	28	0	0	Just Kunai IV
2236	generic	2	4	10	56	40	0	0	Hermit Kunai IV
2237	generic	2	4	11	56	61	0	0	Wheel Kunai IV
2238	generic	2	4	12	56	28	0	0	Might Kunai IV
2239	generic	2	4	13	56	52	0	0	Hanged Kunai IV
2240	generic	2	4	14	56	43	0	0	Reaper Kunai IV
2241	generic	2	4	15	56	111	0	0	Temper Kunai IV
2242	generic	2	4	16	56	46	0	0	Devil Kunai IV
2243	generic	2	4	17	56	96	0	0	Tower Kunai IV
2244	generic	2	4	18	56	105	0	0	Star Kunai IV
2245	generic	2	4	19	56	58	0	0	Moon Kunai IV
2246	generic	2	4	20	56	28	0	0	Sun Kunai IV
2247	generic	2	4	21	56	55	0	0	Judge Kunai IV
2248	generic	2	5	2	59	100	0	0	Magus Kunai V
2249	generic	2	5	3	59	35	0	0	Oracle Kunai V
2250	generic	2	5	4	59	103	0	0	Queen Kunai V
2251	generic	2	5	5	59	109	0	0	King Kunai V
2252	generic	2	5	6	59	111	0	0	Abbot Kunai V
2253	generic	2	5	7	59	38	0	0	Amour Kunai V
2254	generic	2	5	8	59	62	0	0	Rider Kunai V
2255	generic	2	5	9	59	30	0	0	Just Kunai V
2256	generic	2	5	10	59	41	0	0	Hermit Kunai V
2257	generic	2	5	11	59	62	0	0	Wheel Kunai V
2258	generic	2	5	12	59	30	0	0	Might Kunai V
2259	generic	2	5	13	59	53	0	0	Hanged Kunai V
2260	generic	2	5	14	59	44	0	0	Reaper Kunai V
2261	generic	2	5	15	59	112	0	0	Temper Kunai V
2262	generic	2	5	16	59	47	0	0	Devil Kunai V
2263	generic	2	5	17	59	97	0	0	Tower Kunai V
2264	generic	2	5	18	59	106	0	0	Star Kunai V
2265	generic	2	5	19	59	59	0	0	Moon Kunai V
2266	generic	2	5	20	59	30	0	0	Sun Kunai V
2267	generic	2	5	21	59	56	0	0	Judge Kunai V
2268	generic	2	6	2	67	100	0	0	Magus Kunai VI
2269	generic	2	6	3	67	35	0	0	Oracle Kunai VI
2270	generic	2	6	4	67	103	0	0	Queen Kunai VI
2271	generic	2	6	5	67	109	0	0	King Kunai VI
2272	generic	2	6	6	67	111	0	0	Abbot Kunai VI
2273	generic	2	6	7	67	38	0	0	Amour Kunai VI
2274	generic	2	6	8	67	62	0	0	Rider Kunai VI
2275	generic	2	6	9	67	30	0	0	Just Kunai VI
2276	generic	2	6	10	67	41	0	0	Hermit Kunai VI
2277	generic	2	6	11	67	62	0	0	Wheel Kunai VI
2278	generic	2	6	12	67	91	0	0	Might Kunai VI
2279	generic	2	6	13	67	53	0	0	Hanged Kunai VI
2280	generic	2	6	14	67	44	0	0	Reaper Kunai VI
2281	generic	2	6	15	67	112	0	0	Temper Kunai VI
2282	generic	2	6	16	67	47	0	0	Devil Kunai VI
2283	generic	2	6	17	67	97	0	0	Tower Kunai VI
2284	generic	2	6	18	67	106	0	0	Star Kunai VI
2285	generic	2	6	19	67	59	0	0	Moon Kunai VI
2286	generic	2	6	20	67	30	0	0	Sun Kunai VI
2287	generic	2	6	21	67	56	0	0	Judge Kunai VI
2288	generic	3	1	2	114	98	0	0	Magus Boots I
2289	generic	3	1	3	114	31	0	0	Oracle Boots I
2290	generic	3	1	4	114	101	0	0	Queen Boots I
2291	generic	3	1	5	114	107	0	0	King Boots I
2292	generic	3	1	6	114	110	0	0	Abbot Boots I
2293	generic	3	1	7	114	36	0	0	Amour Boots I
2294	generic	3	1	8	114	60	0	0	Rider Boots I
2295	generic	3	1	9	114	26	0	0	Just Boots I
2296	generic	3	1	10	114	39	0	0	Hermit Boots I
2297	generic	3	1	11	114	60	0	0	Wheel Boots I
2298	generic	3	1	12	114	26	0	0	Might Boots I
2299	generic	3	1	13	114	51	0	0	Hanged Boots I
2300	generic	3	1	14	114	42	0	0	Reaper Boots I
2301	generic	3	1	15	114	110	0	0	Temper Boots I
2302	generic	3	1	16	114	45	0	0	Devil Boots I
2303	generic	3	1	17	114	95	0	0	Tower Boots I
2304	generic	3	1	18	114	104	0	0	Star Boots I
2305	generic	3	1	19	114	57	0	0	Moon Boots I
2306	generic	3	1	20	114	26	0	0	Sun Boots I
2307	generic	3	1	21	114	54	0	0	Judge Boots I
2308	generic	3	2	2	121	98	0	0	Magus Boots II
2309	generic	3	2	3	121	31	0	0	Oracle Boots II
2310	generic	3	2	4	121	101	0	0	Queen Boots II
2311	generic	3	2	5	121	107	0	0	King Boots II
2312	generic	3	2	6	121	110	0	0	Abbot Boots II
2313	generic	3	2	7	121	36	0	0	Amour Boots II
2314	generic	3	2	8	121	60	0	0	Rider Boots II
2315	generic	3	2	9	121	26	0	0	Just Boots II
2316	generic	3	2	10	121	39	0	0	Hermit Boots II
2317	generic	3	2	11	121	60	0	0	Wheel Boots II
2318	generic	3	2	12	121	26	0	0	Might Boots II
2319	generic	3	2	13	121	51	0	0	Hanged Boots II
2320	generic	3	2	14	121	42	0	0	Reaper Boots II
2321	generic	3	2	15	121	110	0	0	Temper Boots II
2322	generic	3	2	16	121	45	0	0	Devil Boots II
2323	generic	3	2	17	121	95	0	0	Tower Boots II
2324	generic	3	2	18	121	104	0	0	Star Boots II
2325	generic	3	2	19	121	57	0	0	Moon Boots II
2326	generic	3	2	20	121	26	0	0	Sun Boots II
2327	generic	3	2	21	121	54	0	0	Judge Boots II
2328	generic	3	3	2	123	99	0	0	Magus Boots III
2329	generic	3	3	3	123	33	0	0	Oracle Boots III
2330	generic	3	3	4	123	102	0	0	Queen Boots III
2331	generic	3	3	5	123	108	0	0	King Boots III
2332	generic	3	3	6	123	111	0	0	Abbot Boots III
2333	generic	3	3	7	123	37	0	0	Amour Boots III
2334	generic	3	3	8	123	61	0	0	Rider Boots III
2335	generic	3	3	9	123	28	0	0	Just Boots III
2336	generic	3	3	10	123	40	0	0	Hermit Boots III
2337	generic	3	3	11	123	61	0	0	Wheel Boots III
2338	generic	3	3	12	123	28	0	0	Might Boots III
2339	generic	3	3	13	123	52	0	0	Hanged Boots III
2340	generic	3	3	14	123	43	0	0	Reaper Boots III
2341	generic	3	3	15	123	111	0	0	Temper Boots III
2342	generic	3	3	16	123	46	0	0	Devil Boots III
2343	generic	3	3	17	123	96	0	0	Tower Boots III
2344	generic	3	3	18	123	105	0	0	Star Boots III
2345	generic	3	3	19	123	58	0	0	Moon Boots III
2346	generic	3	3	20	123	28	0	0	Sun Boots III
2347	generic	3	3	21	123	55	0	0	Judge Boots III
2348	generic	3	4	2	127	99	0	0	Magus Boots IV
2349	generic	3	4	3	127	33	0	0	Oracle Boots IV
2350	generic	3	4	4	127	102	0	0	Queen Boots IV
2351	generic	3	4	5	127	108	0	0	King Boots IV
2352	generic	3	4	6	127	111	0	0	Abbot Boots IV
2353	generic	3	4	7	127	37	0	0	Amour Boots IV
2354	generic	3	4	8	127	61	0	0	Rider Boots IV
2355	generic	3	4	9	127	28	0	0	Just Boots IV
2356	generic	3	4	10	127	40	0	0	Hermit Boots IV
2357	generic	3	4	11	127	61	0	0	Wheel Boots IV
2358	generic	3	4	12	127	28	0	0	Might Boots IV
2359	generic	3	4	13	127	52	0	0	Hanged Boots IV
2360	generic	3	4	14	127	43	0	0	Reaper Boots IV
2361	generic	3	4	15	127	111	0	0	Temper Boots IV
2362	generic	3	4	16	127	46	0	0	Devil Boots IV
2363	generic	3	4	17	127	96	0	0	Tower Boots IV
2364	generic	3	4	18	127	105	0	0	Star Boots IV
2365	generic	3	4	19	127	58	0	0	Moon Boots IV
2366	generic	3	4	20	127	28	0	0	Sun Boots IV
2367	generic	3	4	21	127	55	0	0	Judge Boots IV
2368	generic	3	5	2	130	100	0	0	Magus Boots V
2369	generic	3	5	3	130	35	0	0	Oracle Boots V
2370	generic	3	5	4	130	103	0	0	Queen Boots V
2371	generic	3	5	5	130	109	0	0	King Boots V
2372	generic	3	5	6	130	111	0	0	Abbot Boots V
2373	generic	3	5	7	130	38	0	0	Amour Boots V
2374	generic	3	5	8	130	62	0	0	Rider Boots V
2375	generic	3	5	9	130	30	0	0	Just Boots V
2376	generic	3	5	10	130	41	0	0	Hermit Boots V
2377	generic	3	5	11	130	62	0	0	Wheel Boots V
2378	generic	3	5	12	130	30	0	0	Might Boots V
2379	generic	3	5	13	130	53	0	0	Hanged Boots V
2380	generic	3	5	14	130	44	0	0	Reaper Boots V
2381	generic	3	5	15	130	112	0	0	Temper Boots V
2382	generic	3	5	16	130	47	0	0	Devil Boots V
2383	generic	3	5	17	130	97	0	0	Tower Boots V
2384	generic	3	5	18	130	106	0	0	Star Boots V
2385	generic	3	5	19	130	59	0	0	Moon Boots V
2386	generic	3	5	20	130	30	0	0	Sun Boots V
2387	generic	3	5	21	130	56	0	0	Judge Boots V
2388	generic	3	6	2	139	100	0	0	Magus Boots VI
2389	generic	3	6	3	139	35	0	0	Oracle Boots VI
2390	generic	3	6	4	139	103	0	0	Queen Boots VI
2391	generic	3	6	5	139	109	0	0	King Boots VI
2392	generic	3	6	6	139	111	0	0	Abbot Boots VI
2393	generic	3	6	7	139	38	0	0	Amour Boots VI
2394	generic	3	6	8	139	62	0	0	Rider Boots VI
2395	generic	3	6	9	139	30	0	0	Just Boots VI
2396	generic	3	6	10	139	41	0	0	Hermit Boots VI
2397	generic	3	6	11	139	62	0	0	Wheel Boots VI
2398	generic	3	6	12	139	91	0	0	Might Boots VI
2399	generic	3	6	13	139	53	0	0	Hanged Boots VI
2400	generic	3	6	14	139	44	0	0	Reaper Boots VI
2401	generic	3	6	15	139	112	0	0	Temper Boots VI
2402	generic	3	6	16	139	47	0	0	Devil Boots VI
2403	generic	3	6	17	139	97	0	0	Tower Boots VI
2404	generic	3	6	18	139	106	0	0	Star Boots VI
2405	generic	3	6	19	139	59	0	0	Moon Boots VI
2406	generic	3	6	20	139	30	0	0	Sun Boots VI
2407	generic	3	6	21	139	56	0	0	Judge Boots VI
2408	generic	4	1	2	78	98	0	0	Magus Fan I
2409	generic	4	1	3	78	31	0	0	Oracle Fan I
2410	generic	4	1	4	78	101	0	0	Queen Fan I
2411	generic	4	1	5	78	107	0	0	King Fan I
2412	generic	4	1	6	78	110	0	0	Abbot Fan I
2413	generic	4	1	7	78	36	0	0	Amour Fan I
2414	generic	4	1	8	78	60	0	0	Rider Fan I
2415	generic	4	1	9	78	26	0	0	Just Fan I
2416	generic	4	1	10	78	39	0	0	Hermit Fan I
2417	generic	4	1	11	78	60	0	0	Wheel Fan I
2418	generic	4	1	12	78	26	0	0	Might Fan I
2419	generic	4	1	13	78	51	0	0	Hanged Fan I
2420	generic	4	1	14	78	42	0	0	Reaper Fan I
2421	generic	4	1	15	78	110	0	0	Temper Fan I
2422	generic	4	1	16	78	45	0	0	Devil Fan I
2423	generic	4	1	17	78	95	0	0	Tower Fan I
2424	generic	4	1	18	78	104	0	0	Star Fan I
2425	generic	4	1	19	78	57	0	0	Moon Fan I
2426	generic	4	1	20	78	26	0	0	Sun Fan I
2427	generic	4	1	21	78	54	0	0	Judge Fan I
2428	generic	4	2	2	84	98	0	0	Magus Fan II
2429	generic	4	2	3	84	31	0	0	Oracle Fan II
2430	generic	4	2	4	84	101	0	0	Queen Fan II
2431	generic	4	2	5	84	107	0	0	King Fan II
2432	generic	4	2	6	84	110	0	0	Abbot Fan II
2433	generic	4	2	7	84	36	0	0	Amour Fan II
2434	generic	4	2	8	84	60	0	0	Rider Fan II
2435	generic	4	2	9	84	26	0	0	Just Fan II
2436	generic	4	2	10	84	39	0	0	Hermit Fan II
2437	generic	4	2	11	84	60	0	0	Wheel Fan II
2438	generic	4	2	12	84	26	0	0	Might Fan II
2439	generic	4	2	13	84	51	0	0	Hanged Fan II
2440	generic	4	2	14	84	42	0	0	Reaper Fan II
2441	generic	4	2	15	84	110	0	0	Temper Fan II
2442	generic	4	2	16	84	45	0	0	Devil Fan II
2443	generic	4	2	17	84	95	0	0	Tower Fan II
2444	generic	4	2	18	84	104	0	0	Star Fan II
2445	generic	4	2	19	84	57	0	0	Moon Fan II
2446	generic	4	2	20	84	26	0	0	Sun Fan II
2447	generic	4	2	21	84	54	0	0	Judge Fan II
2448	generic	4	3	2	85	99	0	0	Magus Fan III
2449	generic	4	3	3	85	33	0	0	Oracle Fan III
2450	generic	4	3	4	85	102	0	0	Queen Fan III
2451	generic	4	3	5	85	108	0	0	King Fan III
2452	generic	4	3	6	85	111	0	0	Abbot Fan III
2453	generic	4	3	7	85	37	0	0	Amour Fan III
2454	generic	4	3	8	85	61	0	0	Rider Fan III
2455	generic	4	3	9	85	28	0	0	Just Fan III
2456	generic	4	3	10	85	40	0	0	Hermit Fan III
2457	generic	4	3	11	85	61	0	0	Wheel Fan III
2458	generic	4	3	12	85	28	0	0	Might Fan III
2459	generic	4	3	13	85	52	0	0	Hanged Fan III
2460	generic	4	3	14	85	43	0	0	Reaper Fan III
2461	generic	4	3	15	85	111	0	0	Temper Fan III
2462	generic	4	3	16	85	46	0	0	Devil Fan III
2463	generic	4	3	17	85	96	0	0	Tower Fan III
2464	generic	4	3	18	85	105	0	0	Star Fan III
2465	generic	4	3	19	85	58	0	0	Moon Fan III
2466	generic	4	3	20	85	28	0	0	Sun Fan III
2467	generic	4	3	21	85	55	0	0	Judge Fan III
2468	generic	4	4	2	89	99	0	0	Magus Fan IV
2469	generic	4	4	3	89	33	0	0	Oracle Fan IV
2470	generic	4	4	4	89	102	0	0	Queen Fan IV
2471	generic	4	4	5	89	108	0	0	King Fan IV
2472	generic	4	4	6	89	111	0	0	Abbot Fan IV
2473	generic	4	4	7	89	37	0	0	Amour Fan IV
2474	generic	4	4	8	89	61	0	0	Rider Fan IV
2475	generic	4	4	9	89	28	0	0	Just Fan IV
2476	generic	4	4	10	89	40	0	0	Hermit Fan IV
2477	generic	4	4	11	89	61	0	0	Wheel Fan IV
2478	generic	4	4	12	89	28	0	0	Might Fan IV
2479	generic	4	4	13	89	52	0	0	Hanged Fan IV
2480	generic	4	4	14	89	43	0	0	Reaper Fan IV
2481	generic	4	4	15	89	111	0	0	Temper Fan IV
2482	generic	4	4	16	89	46	0	0	Devil Fan IV
2483	generic	4	4	17	89	96	0	0	Tower Fan IV
2484	generic	4	4	18	89	105	0	0	Star Fan IV
2485	generic	4	4	19	89	58	0	0	Moon Fan IV
2486	generic	4	4	20	89	28	0	0	Sun Fan IV
2487	generic	4	4	21	89	55	0	0	Judge Fan IV
2488	generic	4	5	2	94	100	0	0	Magus Fan V
2489	generic	4	5	3	94	35	0	0	Oracle Fan V
2490	generic	4	5	4	94	103	0	0	Queen Fan V
2491	generic	4	5	5	94	109	0	0	King Fan V
2492	generic	4	5	6	94	111	0	0	Abbot Fan V
2493	generic	4	5	7	94	38	0	0	Amour Fan V
2494	generic	4	5	8	94	62	0	0	Rider Fan V
2495	generic	4	5	9	94	30	0	0	Just Fan V
2496	generic	4	5	10	94	41	0	0	Hermit Fan V
2497	generic	4	5	11	94	62	0	0	Wheel Fan V
2498	generic	4	5	12	94	30	0	0	Might Fan V
2499	generic	4	5	13	94	53	0	0	Hanged Fan V
2500	generic	4	5	14	94	44	0	0	Reaper Fan V
2501	generic	4	5	15	94	112	0	0	Temper Fan V
2502	generic	4	5	16	94	47	0	0	Devil Fan V
2503	generic	4	5	17	94	97	0	0	Tower Fan V
2504	generic	4	5	18	94	106	0	0	Star Fan V
2505	generic	4	5	19	94	59	0	0	Moon Fan V
2506	generic	4	5	20	94	30	0	0	Sun Fan V
2507	generic	4	5	21	94	56	0	0	Judge Fan V
2508	generic	4	6	2	101	100	0	0	Magus Fan VI
2509	generic	4	6	3	101	35	0	0	Oracle Fan VI
2510	generic	4	6	4	101	103	0	0	Queen Fan VI
2511	generic	4	6	5	101	109	0	0	King Fan VI
2512	generic	4	6	6	101	111	0	0	Abbot Fan VI
2513	generic	4	6	7	101	38	0	0	Amour Fan VI
2514	generic	4	6	8	101	62	0	0	Rider Fan VI
2515	generic	4	6	9	101	30	0	0	Just Fan VI
2516	generic	4	6	10	101	41	0	0	Hermit Fan VI
2517	generic	4	6	11	101	62	0	0	Wheel Fan VI
2518	generic	4	6	12	101	91	0	0	Might Fan VI
2519	generic	4	6	13	101	53	0	0	Hanged Fan VI
2520	generic	4	6	14	101	44	0	0	Reaper Fan VI
2521	generic	4	6	15	101	112	0	0	Temper Fan VI
2522	generic	4	6	16	101	47	0	0	Devil Fan VI
2523	generic	4	6	17	101	97	0	0	Tower Fan VI
2524	generic	4	6	18	101	106	0	0	Star Fan VI
2525	generic	4	6	19	101	59	0	0	Moon Fan VI
2526	generic	4	6	20	101	30	0	0	Sun Fan VI
2527	generic	4	6	21	101	56	0	0	Judge Fan VI
2528	generic	6	1	2	151	98	0	0	Magus Guard I
2529	generic	6	1	3	151	31	0	0	Oracle Guard I
2530	generic	6	1	4	151	101	0	0	Queen Guard I
2531	generic	6	1	5	151	107	0	0	King Guard I
2532	generic	6	1	6	151	110	0	0	Abbot Guard I
2533	generic	6	1	7	151	36	0	0	Amour Guard I
2534	generic	6	1	8	151	60	0	0	Rider Guard I
2535	generic	6	1	9	151	26	0	0	Just Guard I
2536	generic	6	1	10	151	39	0	0	Hermit Guard I
2537	generic	6	1	11	151	60	0	0	Wheel Guard I
2538	generic	6	1	12	151	26	0	0	Might Guard I
2539	generic	6	1	13	151	51	0	0	Hanged Guard I
2540	generic	6	1	14	151	42	0	0	Reaper Guard I
2541	generic	6	1	15	151	110	0	0	Temper Guard I
2542	generic	6	1	16	151	45	0	0	Devil Guard I
2543	generic	6	1	17	151	95	0	0	Tower Guard I
2544	generic	6	1	18	151	104	0	0	Star Guard I
2545	generic	6	1	19	151	57	0	0	Moon Guard I
2546	generic	6	1	20	151	26	0	0	Sun Guard I
2547	generic	6	1	21	151	54	0	0	Judge Guard I
2548	generic	6	2	2	151	98	0	0	Magus Guard II
2549	generic	6	2	3	151	31	0	0	Oracle Guard II
2550	generic	6	2	4	151	101	0	0	Queen Guard II
2551	generic	6	2	5	151	107	0	0	King Guard II
2552	generic	6	2	6	151	110	0	0	Abbot Guard II
2553	generic	6	2	7	151	36	0	0	Amour Guard II
2554	generic	6	2	8	151	60	0	0	Rider Guard II
2555	generic	6	2	9	151	26	0	0	Just Guard II
2556	generic	6	2	10	151	39	0	0	Hermit Guard II
2557	generic	6	2	11	151	60	0	0	Wheel Guard II
2558	generic	6	2	12	151	26	0	0	Might Guard II
2559	generic	6	2	13	151	51	0	0	Hanged Guard II
2560	generic	6	2	14	151	42	0	0	Reaper Guard II
2561	generic	6	2	15	151	110	0	0	Temper Guard II
2562	generic	6	2	16	151	45	0	0	Devil Guard II
2563	generic	6	2	17	151	95	0	0	Tower Guard II
2564	generic	6	2	18	151	104	0	0	Star Guard II
2565	generic	6	2	19	151	57	0	0	Moon Guard II
2566	generic	6	2	20	151	26	0	0	Sun Guard II
2567	generic	6	2	21	151	54	0	0	Judge Guard II
2568	generic	6	3	2	154	99	0	0	Magus Guard III
2569	generic	6	3	3	154	33	0	0	Oracle Guard III
2570	generic	6	3	4	154	102	0	0	Queen Guard III
2571	generic	6	3	5	154	108	0	0	King Guard III
2572	generic	6	3	6	154	111	0	0	Abbot Guard III
2573	generic	6	3	7	154	37	0	0	Amour Guard III
2574	generic	6	3	8	154	61	0	0	Rider Guard III
2575	generic	6	3	9	154	28	0	0	Just Guard III
2576	generic	6	3	10	154	40	0	0	Hermit Guard III
2577	generic	6	3	11	154	61	0	0	Wheel Guard III
2578	generic	6	3	12	154	28	0	0	Might Guard III
2579	generic	6	3	13	154	52	0	0	Hanged Guard III
2580	generic	6	3	14	154	43	0	0	Reaper Guard III
2581	generic	6	3	15	154	111	0	0	Temper Guard III
2582	generic	6	3	16	154	46	0	0	Devil Guard III
2583	generic	6	3	17	154	96	0	0	Tower Guard III
2584	generic	6	3	18	154	105	0	0	Star Guard III
2585	generic	6	3	19	154	58	0	0	Moon Guard III
2586	generic	6	3	20	154	28	0	0	Sun Guard III
2587	generic	6	3	21	154	55	0	0	Judge Guard III
2588	generic	6	4	2	159	99	0	0	Magus Guard IV
2589	generic	6	4	3	159	33	0	0	Oracle Guard IV
2590	generic	6	4	4	159	102	0	0	Queen Guard IV
2591	generic	6	4	5	159	108	0	0	King Guard IV
2592	generic	6	4	6	159	111	0	0	Abbot Guard IV
2593	generic	6	4	7	159	37	0	0	Amour Guard IV
2594	generic	6	4	8	159	61	0	0	Rider Guard IV
2595	generic	6	4	9	159	28	0	0	Just Guard IV
2596	generic	6	4	10	159	40	0	0	Hermit Guard IV
2597	generic	6	4	11	159	61	0	0	Wheel Guard IV
2598	generic	6	4	12	159	28	0	0	Might Guard IV
2599	generic	6	4	13	159	52	0	0	Hanged Guard IV
2600	generic	6	4	14	159	43	0	0	Reaper Guard IV
2601	generic	6	4	15	159	111	0	0	Temper Guard IV
2602	generic	6	4	16	159	46	0	0	Devil Guard IV
2603	generic	6	4	17	159	96	0	0	Tower Guard IV
2604	generic	6	4	18	159	105	0	0	Star Guard IV
2605	generic	6	4	19	159	58	0	0	Moon Guard IV
2606	generic	6	4	20	159	28	0	0	Sun Guard IV
2607	generic	6	4	21	159	55	0	0	Judge Guard IV
2608	generic	6	5	2	162	100	0	0	Magus Guard V
2609	generic	6	5	3	162	35	0	0	Oracle Guard V
2610	generic	6	5	4	162	103	0	0	Queen Guard V
2611	generic	6	5	5	162	109	0	0	King Guard V
2612	generic	6	5	6	162	111	0	0	Abbot Guard V
2613	generic	6	5	7	162	38	0	0	Amour Guard V
2614	generic	6	5	8	162	62	0	0	Rider Guard V
2615	generic	6	5	9	162	30	0	0	Just Guard V
2616	generic	6	5	10	162	41	0	0	Hermit Guard V
2617	generic	6	5	11	162	62	0	0	Wheel Guard V
2618	generic	6	5	12	162	30	0	0	Might Guard V
2619	generic	6	5	13	162	53	0	0	Hanged Guard V
2620	generic	6	5	14	162	44	0	0	Reaper Guard V
2621	generic	6	5	15	162	112	0	0	Temper Guard V
2622	generic	6	5	16	162	47	0	0	Devil Guard V
2623	generic	6	5	17	162	97	0	0	Tower Guard V
2624	generic	6	5	18	162	106	0	0	Star Guard V
2625	generic	6	5	19	162	59	0	0	Moon Guard V
2626	generic	6	5	20	162	30	0	0	Sun Guard V
2627	generic	6	5	21	162	56	0	0	Judge Guard V
2628	generic	6	6	2	171	100	0	0	Magus Guard VI
2629	generic	6	6	3	171	35	0	0	Oracle Guard VI
2630	generic	6	6	4	171	103	0	0	Queen Guard VI
2631	generic	6	6	5	171	109	0	0	King Guard VI
2632	generic	6	6	6	171	111	0	0	Abbot Guard VI
2633	generic	6	6	7	171	38	0	0	Amour Guard VI
2634	generic	6	6	8	171	62	0	0	Rider Guard VI
2635	generic	6	6	9	171	30	0	0	Just Guard VI
2636	generic	6	6	10	171	41	0	0	Hermit Guard VI
2637	generic	6	6	11	171	62	0	0	Wheel Guard VI
2638	generic	6	6	12	171	91	0	0	Might Guard VI
2639	generic	6	6	13	171	53	0	0	Hanged Guard VI
2640	generic	6	6	14	171	44	0	0	Reaper Guard VI
2641	generic	6	6	15	171	112	0	0	Temper Guard VI
2642	generic	6	6	16	171	47	0	0	Devil Guard VI
2643	generic	6	6	17	171	97	0	0	Tower Guard VI
2644	generic	6	6	18	171	106	0	0	Star Guard VI
2645	generic	6	6	19	171	59	0	0	Moon Guard VI
2646	generic	6	6	20	171	30	0	0	Sun Guard VI
2647	generic	6	6	21	171	56	0	0	Judge Guard VI
2648	generic	7	1	2	186	98	0	0	Magus Gun I
2649	generic	7	1	3	186	31	0	0	Oracle Gun I
2650	generic	7	1	4	186	101	0	0	Queen Gun I
2651	generic	7	1	5	186	107	0	0	King Gun I
2652	generic	7	1	6	186	110	0	0	Abbot Gun I
2653	generic	7	1	7	186	36	0	0	Amour Gun I
2654	generic	7	1	8	186	60	0	0	Rider Gun I
2655	generic	7	1	9	186	26	0	0	Just Gun I
2656	generic	7	1	10	186	39	0	0	Hermit Gun I
2657	generic	7	1	11	186	60	0	0	Wheel Gun I
2658	generic	7	1	12	186	26	0	0	Might Gun I
2659	generic	7	1	13	186	51	0	0	Hanged Gun I
2660	generic	7	1	14	186	42	0	0	Reaper Gun I
2661	generic	7	1	15	186	110	0	0	Temper Gun I
2662	generic	7	1	16	186	45	0	0	Devil Gun I
2663	generic	7	1	17	186	95	0	0	Tower Gun I
2664	generic	7	1	18	186	104	0	0	Star Gun I
2665	generic	7	1	19	186	57	0	0	Moon Gun I
2666	generic	7	1	20	186	26	0	0	Sun Gun I
2667	generic	7	1	21	186	54	0	0	Judge Gun I
2668	generic	7	2	2	186	98	0	0	Magus Gun II
2669	generic	7	2	3	186	31	0	0	Oracle Gun II
2670	generic	7	2	4	186	101	0	0	Queen Gun II
2671	generic	7	2	5	186	107	0	0	King Gun II
2672	generic	7	2	6	186	110	0	0	Abbot Gun II
2673	generic	7	2	7	186	36	0	0	Amour Gun II
2674	generic	7	2	8	186	60	0	0	Rider Gun II
2675	generic	7	2	9	186	26	0	0	Just Gun II
2676	generic	7	2	10	186	39	0	0	Hermit Gun II
2677	generic	7	2	11	186	60	0	0	Wheel Gun II
2678	generic	7	2	12	186	26	0	0	Might Gun II
2679	generic	7	2	13	186	51	0	0	Hanged Gun II
2680	generic	7	2	14	186	42	0	0	Reaper Gun II
2681	generic	7	2	15	186	110	0	0	Temper Gun II
2682	generic	7	2	16	186	45	0	0	Devil Gun II
2683	generic	7	2	17	186	95	0	0	Tower Gun II
2684	generic	7	2	18	186	104	0	0	Star Gun II
2685	generic	7	2	19	186	57	0	0	Moon Gun II
2686	generic	7	2	20	186	26	0	0	Sun Gun II
2687	generic	7	2	21	186	54	0	0	Judge Gun II
2688	generic	7	3	2	186	99	0	0	Magus Gun III
2689	generic	7	3	3	186	33	0	0	Oracle Gun III
2690	generic	7	3	4	186	102	0	0	Queen Gun III
2691	generic	7	3	5	186	108	0	0	King Gun III
2692	generic	7	3	6	186	111	0	0	Abbot Gun III
2693	generic	7	3	7	186	37	0	0	Amour Gun III
2694	generic	7	3	8	186	61	0	0	Rider Gun III
2695	generic	7	3	9	186	28	0	0	Just Gun III
2696	generic	7	3	10	186	40	0	0	Hermit Gun III
2697	generic	7	3	11	186	61	0	0	Wheel Gun III
2698	generic	7	3	12	186	28	0	0	Might Gun III
2699	generic	7	3	13	186	52	0	0	Hanged Gun III
2700	generic	7	3	14	186	43	0	0	Reaper Gun III
2701	generic	7	3	15	186	111	0	0	Temper Gun III
2702	generic	7	3	16	186	46	0	0	Devil Gun III
2703	generic	7	3	17	186	96	0	0	Tower Gun III
2704	generic	7	3	18	186	105	0	0	Star Gun III
2705	generic	7	3	19	186	58	0	0	Moon Gun III
2706	generic	7	3	20	186	28	0	0	Sun Gun III
2707	generic	7	3	21	186	55	0	0	Judge Gun III
2708	generic	7	4	2	186	99	0	0	Magus Gun IV
2709	generic	7	4	3	186	33	0	0	Oracle Gun IV
2710	generic	7	4	4	186	102	0	0	Queen Gun IV
2711	generic	7	4	5	186	108	0	0	King Gun IV
2712	generic	7	4	6	186	111	0	0	Abbot Gun IV
2713	generic	7	4	7	186	37	0	0	Amour Gun IV
2714	generic	7	4	8	186	61	0	0	Rider Gun IV
2715	generic	7	4	9	186	28	0	0	Just Gun IV
2716	generic	7	4	10	186	40	0	0	Hermit Gun IV
2717	generic	7	4	11	186	61	0	0	Wheel Gun IV
2718	generic	7	4	12	186	28	0	0	Might Gun IV
2719	generic	7	4	13	186	52	0	0	Hanged Gun IV
2720	generic	7	4	14	186	43	0	0	Reaper Gun IV
2721	generic	7	4	15	186	111	0	0	Temper Gun IV
2722	generic	7	4	16	186	46	0	0	Devil Gun IV
2723	generic	7	4	17	186	96	0	0	Tower Gun IV
2724	generic	7	4	18	186	105	0	0	Star Gun IV
2725	generic	7	4	19	186	58	0	0	Moon Gun IV
2726	generic	7	4	20	186	28	0	0	Sun Gun IV
2727	generic	7	4	21	186	55	0	0	Judge Gun IV
2728	generic	7	5	2	186	100	0	0	Magus Gun V
2729	generic	7	5	3	186	35	0	0	Oracle Gun V
2730	generic	7	5	4	186	103	0	0	Queen Gun V
2731	generic	7	5	5	186	109	0	0	King Gun V
2732	generic	7	5	6	186	111	0	0	Abbot Gun V
2733	generic	7	5	7	186	38	0	0	Amour Gun V
2734	generic	7	5	8	186	62	0	0	Rider Gun V
2735	generic	7	5	9	186	30	0	0	Just Gun V
2736	generic	7	5	10	186	41	0	0	Hermit Gun V
2737	generic	7	5	11	186	62	0	0	Wheel Gun V
2738	generic	7	5	12	186	30	0	0	Might Gun V
2739	generic	7	5	13	186	53	0	0	Hanged Gun V
2740	generic	7	5	14	186	44	0	0	Reaper Gun V
2741	generic	7	5	15	186	112	0	0	Temper Gun V
2742	generic	7	5	16	186	47	0	0	Devil Gun V
2743	generic	7	5	17	186	97	0	0	Tower Gun V
2744	generic	7	5	18	186	106	0	0	Star Gun V
2745	generic	7	5	19	186	59	0	0	Moon Gun V
2746	generic	7	5	20	186	30	0	0	Sun Gun V
2747	generic	7	5	21	186	56	0	0	Judge Gun V
2748	generic	7	6	2	196	100	0	0	Magus Gun VI
2749	generic	7	6	3	196	35	0	0	Oracle Gun VI
2750	generic	7	6	4	196	103	0	0	Queen Gun VI
2751	generic	7	6	5	196	109	0	0	King Gun VI
2752	generic	7	6	6	196	111	0	0	Abbot Gun VI
2753	generic	7	6	7	196	38	0	0	Amour Gun VI
2754	generic	7	6	8	196	62	0	0	Rider Gun VI
2755	generic	7	6	9	196	30	0	0	Just Gun VI
2756	generic	7	6	10	196	41	0	0	Hermit Gun VI
2757	generic	7	6	11	196	62	0	0	Wheel Gun VI
2758	generic	7	6	12	196	91	0	0	Might Gun VI
2759	generic	7	6	13	196	53	0	0	Hanged Gun VI
2760	generic	7	6	14	196	44	0	0	Reaper Gun VI
2761	generic	7	6	15	196	112	0	0	Temper Gun VI
2762	generic	7	6	16	196	47	0	0	Devil Gun VI
2763	generic	7	6	17	196	97	0	0	Tower Gun VI
2764	generic	7	6	18	196	106	0	0	Star Gun VI
2765	generic	7	6	19	196	59	0	0	Moon Gun VI
2766	generic	7	6	20	196	30	0	0	Sun Gun VI
2767	generic	7	6	21	196	56	0	0	Judge Gun VI
2768	generic	8	1	2	220	98	0	0	Magus Claws I
2769	generic	8	1	3	220	31	0	0	Oracle Claws I
2770	generic	8	1	4	220	101	0	0	Queen Claws I
2771	generic	8	1	5	220	107	0	0	King Claws I
2772	generic	8	1	6	220	110	0	0	Abbot Claws I
2773	generic	8	1	7	220	36	0	0	Amour Claws I
2774	generic	8	1	8	220	60	0	0	Rider Claws I
2775	generic	8	1	9	220	26	0	0	Just Claws I
2776	generic	8	1	10	220	39	0	0	Hermit Claws I
2777	generic	8	1	11	220	60	0	0	Wheel Claws I
2778	generic	8	1	12	220	26	0	0	Might Claws I
2779	generic	8	1	13	220	51	0	0	Hanged Claws I
2780	generic	8	1	14	220	42	0	0	Reaper Claws I
2781	generic	8	1	15	220	110	0	0	Temper Claws I
2782	generic	8	1	16	220	45	0	0	Devil Claws I
2783	generic	8	1	17	220	95	0	0	Tower Claws I
2784	generic	8	1	18	220	104	0	0	Star Claws I
2785	generic	8	1	19	220	57	0	0	Moon Claws I
2786	generic	8	1	20	220	26	0	0	Sun Claws I
2787	generic	8	1	21	220	54	0	0	Judge Claws I
2788	generic	8	2	2	220	98	0	0	Magus Claws II
2789	generic	8	2	3	220	31	0	0	Oracle Claws II
2790	generic	8	2	4	220	101	0	0	Queen Claws II
2791	generic	8	2	5	220	107	0	0	King Claws II
2792	generic	8	2	6	220	110	0	0	Abbot Claws II
2793	generic	8	2	7	220	36	0	0	Amour Claws II
2794	generic	8	2	8	220	60	0	0	Rider Claws II
2795	generic	8	2	9	220	26	0	0	Just Claws II
2796	generic	8	2	10	220	39	0	0	Hermit Claws II
2797	generic	8	2	11	220	60	0	0	Wheel Claws II
2798	generic	8	2	12	220	26	0	0	Might Claws II
2799	generic	8	2	13	220	51	0	0	Hanged Claws II
2800	generic	8	2	14	220	42	0	0	Reaper Claws II
2801	generic	8	2	15	220	110	0	0	Temper Claws II
2802	generic	8	2	16	220	45	0	0	Devil Claws II
2803	generic	8	2	17	220	95	0	0	Tower Claws II
2804	generic	8	2	18	220	104	0	0	Star Claws II
2805	generic	8	2	19	220	57	0	0	Moon Claws II
2806	generic	8	2	20	220	26	0	0	Sun Claws II
2807	generic	8	2	21	220	54	0	0	Judge Claws II
2808	generic	8	3	2	220	99	0	0	Magus Claws III
2809	generic	8	3	3	220	33	0	0	Oracle Claws III
2810	generic	8	3	4	220	102	0	0	Queen Claws III
2811	generic	8	3	5	220	108	0	0	King Claws III
2812	generic	8	3	6	220	111	0	0	Abbot Claws III
2813	generic	8	3	7	220	37	0	0	Amour Claws III
2814	generic	8	3	8	220	61	0	0	Rider Claws III
2815	generic	8	3	9	220	28	0	0	Just Claws III
2816	generic	8	3	10	220	40	0	0	Hermit Claws III
2817	generic	8	3	11	220	61	0	0	Wheel Claws III
2818	generic	8	3	12	220	28	0	0	Might Claws III
2819	generic	8	3	13	220	52	0	0	Hanged Claws III
2820	generic	8	3	14	220	43	0	0	Reaper Claws III
2821	generic	8	3	15	220	111	0	0	Temper Claws III
2822	generic	8	3	16	220	46	0	0	Devil Claws III
2823	generic	8	3	17	220	96	0	0	Tower Claws III
2824	generic	8	3	18	220	105	0	0	Star Claws III
2825	generic	8	3	19	220	58	0	0	Moon Claws III
2826	generic	8	3	20	220	28	0	0	Sun Claws III
2827	generic	8	3	21	220	55	0	0	Judge Claws III
2828	generic	8	4	2	224	99	0	0	Magus Claws IV
2829	generic	8	4	3	224	33	0	0	Oracle Claws IV
2830	generic	8	4	4	224	102	0	0	Queen Claws IV
2831	generic	8	4	5	224	108	0	0	King Claws IV
2832	generic	8	4	6	224	111	0	0	Abbot Claws IV
2833	generic	8	4	7	224	37	0	0	Amour Claws IV
2834	generic	8	4	8	224	61	0	0	Rider Claws IV
2835	generic	8	4	9	224	28	0	0	Just Claws IV
2836	generic	8	4	10	224	40	0	0	Hermit Claws IV
2837	generic	8	4	11	224	61	0	0	Wheel Claws IV
2838	generic	8	4	12	224	28	0	0	Might Claws IV
2839	generic	8	4	13	224	52	0	0	Hanged Claws IV
2840	generic	8	4	14	224	43	0	0	Reaper Claws IV
2841	generic	8	4	15	224	111	0	0	Temper Claws IV
2842	generic	8	4	16	224	46	0	0	Devil Claws IV
2843	generic	8	4	17	224	96	0	0	Tower Claws IV
2844	generic	8	4	18	224	105	0	0	Star Claws IV
2845	generic	8	4	19	224	58	0	0	Moon Claws IV
2846	generic	8	4	20	224	28	0	0	Sun Claws IV
2847	generic	8	4	21	224	55	0	0	Judge Claws IV
2848	generic	8	5	2	226	100	0	0	Magus Claws V
2849	generic	8	5	3	226	35	0	0	Oracle Claws V
2850	generic	8	5	4	226	103	0	0	Queen Claws V
2851	generic	8	5	5	226	109	0	0	King Claws V
2852	generic	8	5	6	226	111	0	0	Abbot Claws V
2853	generic	8	5	7	226	38	0	0	Amour Claws V
2854	generic	8	5	8	226	62	0	0	Rider Claws V
2855	generic	8	5	9	226	30	0	0	Just Claws V
2856	generic	8	5	10	226	41	0	0	Hermit Claws V
2857	generic	8	5	11	226	62	0	0	Wheel Claws V
2858	generic	8	5	12	226	30	0	0	Might Claws V
2859	generic	8	5	13	226	53	0	0	Hanged Claws V
2860	generic	8	5	14	226	44	0	0	Reaper Claws V
2861	generic	8	5	15	226	112	0	0	Temper Claws V
2862	generic	8	5	16	226	47	0	0	Devil Claws V
2863	generic	8	5	17	226	97	0	0	Tower Claws V
2864	generic	8	5	18	226	106	0	0	Star Claws V
2865	generic	8	5	19	226	59	0	0	Moon Claws V
2866	generic	8	5	20	226	30	0	0	Sun Claws V
2867	generic	8	5	21	226	56	0	0	Judge Claws V
2868	generic	8	6	2	237	100	0	0	Magus Claws VI
2869	generic	8	6	3	237	35	0	0	Oracle Claws VI
2870	generic	8	6	4	237	103	0	0	Queen Claws VI
2871	generic	8	6	5	237	109	0	0	King Claws VI
2872	generic	8	6	6	237	111	0	0	Abbot Claws VI
2873	generic	8	6	7	237	38	0	0	Amour Claws VI
2874	generic	8	6	8	237	62	0	0	Rider Claws VI
2875	generic	8	6	9	237	30	0	0	Just Claws VI
2876	generic	8	6	10	237	41	0	0	Hermit Claws VI
2877	generic	8	6	11	237	62	0	0	Wheel Claws VI
2878	generic	8	6	12	237	91	0	0	Might Claws VI
2879	generic	8	6	13	237	53	0	0	Hanged Claws VI
2880	generic	8	6	14	237	44	0	0	Reaper Claws VI
2881	generic	8	6	15	237	112	0	0	Temper Claws VI
2882	generic	8	6	16	237	47	0	0	Devil Claws VI
2883	generic	8	6	17	237	97	0	0	Tower Claws VI
2884	generic	8	6	18	237	106	0	0	Star Claws VI
2885	generic	8	6	19	237	59	0	0	Moon Claws VI
2886	generic	8	6	20	237	30	0	0	Sun Claws VI
2887	generic	8	6	21	237	56	0	0	Judge Claws VI
2888	wild	1	1	25	6	91	0	1	Obariyon Blade
2889	wild	1	2	129	6	138	0	1	Legion Blade
2890	wild	1	3	12	6	62	0	1	Ose Blade
2891	wild	1	3	119	6	103	0	1	Black Frost Blad
2892	wild	1	4	65	8	112	0	1	Decarabia Blade
2893	wild	1	5	4	9	38	0	1	Loki Blade
2894	wild	1	1	67	6	44	0	1	Yomotsu Blade
2895	wild	1	5	106	9	59	0	1	Shiki Blade
2896	wild	2	1	25	43	91	0	1	Obariyon Kunai
2897	wild	2	2	129	43	138	0	1	Legion Kunai
2898	wild	2	3	12	43	62	0	1	Ose Kunai
2899	wild	2	3	119	43	103	0	1	Black Frost Kuna
2900	wild	2	4	65	44	112	0	1	Decarabia Kunai
2901	wild	2	5	4	46	38	0	1	Loki Kunai
2902	wild	2	1	67	43	44	0	1	Yomotsu Kunai
2903	wild	2	5	106	46	59	0	1	Shiki Kunai
2904	wild	3	1	25	114	91	0	1	Obariyon Boots
2905	wild	3	2	129	114	138	0	1	Legion Boots
2906	wild	3	3	12	114	62	0	1	Ose Boots
2907	wild	3	3	119	114	103	0	1	Black Frost Boot
2908	wild	3	4	65	115	112	0	1	Decarabia Boots
2909	wild	3	5	4	116	38	0	1	Loki Boots
2910	wild	3	1	67	114	44	0	1	Yomotsu Boots
2911	wild	3	5	106	116	59	0	1	Shiki Boots
2912	wild	4	1	25	78	91	0	1	Obariyon Fan
2913	wild	4	2	129	78	138	0	1	Legion Fan
2914	wild	4	3	12	78	62	0	1	Ose Fan
2915	wild	4	3	119	78	103	0	1	Black Frost Fan
2916	wild	4	4	65	78	112	0	1	Decarabia Fan
2917	wild	4	5	4	78	38	0	1	Loki Fan
2918	wild	4	1	67	78	44	0	1	Yomotsu Fan
2919	wild	4	5	106	78	59	0	1	Shiki Fan
2920	wild	6	1	25	151	91	0	1	Obariyon Guard
2921	wild	6	2	129	151	138	0	1	Legion Guard
2922	wild	6	3	12	151	62	0	1	Ose Guard
2923	wild	6	3	119	151	103	0	1	Black Frost Guar
2924	wild	6	4	65	151	112	0	1	Decarabia Guard
2925	wild	6	5	4	151	38	0	1	Loki Guard
2926	wild	6	1	67	151	44	0	1	Yomotsu Guard
2927	wild	6	5	106	151	59	0	1	Shiki Guard
2928	wild	7	1	25	185	91	0	1	Obariyon Gun
2929	wild	7	2	129	185	138	0	1	Legion Gun
2930	wild	7	3	12	185	62	0	1	Ose Gun
2931	wild	7	3	119	185	103	0	1	Black Frost Gun
2932	wild	7	4	65	185	112	0	1	Decarabia Gun
2933	wild	7	5	4	185	38	0	1	Loki Gun
2934	wild	7	1	67	185	44	0	1	Yomotsu Gun
2935	wild	7	5	106	185	59	0	1	Shiki Gun
2936	wild	8	1	25	220	91	0	1	Obariyon Claws
2937	wild	8	2	129	220	138	0	1	Legion Claws
2938	wild	8	3	12	220	62	0	1	Ose Claws
2939	wild	8	3	119	220	103	0	1	Black Frost Claw
2940	wild	8	4	65	220	112	0	1	Decarabia Claws
2941	wild	8	5	4	220	38	0	1	Loki Claws
2942	wild	8	1	67	220	44	0	1	Yomotsu Claws
2943	wild	8	5	106	220	59	0	1	Shiki Claws
2944	signature	1	2	63	11	28	0	0	Smith-God Blade
2945	signature	1	6	38	32	62	0	0	Ever-Keen Blade
2946	signature	2	4	175	50	0	4	0	Tengu Gale Kunai
2947	signature	2	6	147	67	62	0	0	Hidden-Oni Kunai
2948	signature	3	4	81	127	62	0	0	Ape-Lord Boots
2949	signature	3	4	138	130	96	0	0	Iron-Oni Boots
2950	signature	4	4	100	82	0	1	0	Vermilion Fan
2951	signature	4	4	17	89	138	0	0	Mediator's Fan
2952	signature	6	3	154	154	61	0	0	Oni Iron Guard
2953	signature	6	5	77	155	0	3	0	Storm Maul
2954	signature	8	4	136	224	62	0	0	Man-Lion Claws
2955	signature	8	5	99	223	0	2	0	White-Tiger Claw
2956	signature	7	6	10	196	0	0	1	War-God Pistol
2957	signature	7	6	28	196	62	0	1	Never-Miss Gun
]]
local VESSELS = [[
1	1	2	3	4	5	6	7	8	9	10	11	12	13	14	15	16	17	18	19	20	21	22	23	24	25	26	27	28	29	30	31	32	33	34	35	36
2	39	40	41	42	43	44	45	46	47	48	49	50	51	52	53	54	55	56	57	58	59	60	61	62	63	64	65	66	67	68	69	70
3	112	113	114	115	116	117	118	119	120	121	122	123	124	125	126	127	128	129	130	131	132	133	134	135	136	137	138	139	140	141	142
4	77	78	79	80	81	82	83	84	85	86	87	88	89	90	91	92	93	94	95	96	97	98	99	100	101	102	103	104
6	150	151	152	153	154	155	156	157	158	159	160	161	162	163	164	165	166	167	168	169	170	171	172	173	174	175
7	183	184	185	186	187	188	189	190	191	192	193	194	195	196	197
8	217	218	219	220	221	222	223	224	225	226	227	228	229	230	231	232	233	234	235	236	237	238
]]
-- END GENERATED CATALOG

local rows: {[number]: Row} = {}
local generic: {[string]: Row} = {}
local wild: {[string]: Row} = {}
local signatures: {[number]: Row} = {}
local vessels: {[number]: {number}} = {}
for line in string.gmatch(CATALOG, "[^\n]+") do
    local f = string.split(line, "\t")
    local row: Row = {id = tonumber(f[1]) :: number, kind = f[2], character = tonumber(f[3]) :: number,
        band = tonumber(f[4]) :: number, key = tonumber(f[5]) :: number, anchor = tonumber(f[6]) :: number,
        value = tonumber(f[7]) :: number, element = tonumber(f[8]) :: number, hit = f[9] == "1", name = f[10]}
    rows[row.id] = row
    if row.kind == "generic" then
        generic[row.character .. ":" .. row.band .. ":" .. row.key] = row
    elseif row.kind == "wild" then
        wild[row.character .. ":" .. row.key] = row
    else
        signatures[row.key] = row
    end
end
for line in string.gmatch(VESSELS, "[^\n]+") do
    local f = string.split(line, "\t")
    local list = {}
    for i = 2, #f do table.insert(list, tonumber(f[i]) :: number) end
    vessels[tonumber(f[1]) :: number] = list
end
-- Forged generic and Wild weapons are vessels too (re-forging); signatures are not.
for id = 2048, 3327 do
    local row = rows[id]
    if row and row.kind ~= "signature" then table.insert(vessels[row.character], id) end
end

local function band(level: number): number
    if level <= 14 then return 1 elseif level <= 26 then return 2 elseif level <= 40 then return 3
    elseif level <= 54 then return 4 elseif level <= 69 then return 5 end
    return 6
end

-- What a Persona forges for a fighter, or nil and the reason it cannot.
local function result(character: Character, entry: P4StockEntry): (Row?, string?)
    local signature = signatures[entry.persona]
    if signature and signature.character == character.id then
        local ready = if character.id == 1 then entry.registered else p4.social.rank(character.arcana) >= SIGNATURE_RANK
        if signature.element ~= 0 then
            local level = p4.party.hero_level()
            if level == nil or level < ELEMENTAL_LEVEL then ready = false end
        end
        if ready then return signature, nil end
    end
    if entry.arcana == 1 then
        local row = wild[character.id .. ":" .. entry.persona]
        if row then return row, nil end
        return nil, "This Fool Persona makes no Wild weapon."
    end
    local row = generic[character.id .. ":" .. band(entry.level) .. ":" .. entry.arcana]
    if row then return row, nil end
    return nil, "Personas of this arcana cannot be forged."
end

local function itemName(item: number): string
    local row = rows[item]
    if row then return row.name end
    return p4.data.item(item).name or ("ITEM " .. item)
end
local function personaName(persona: number): string
    return p4.data.persona(persona).name or ("PERSONA " .. persona)
end
-- How many unequipped copies the fighter holds of item as a vessel: the forge's own
-- vessel check (p4.forge.vessel: a known weapon row the fighter may equip), which never
-- reads the display name, so a weapon another mod renamed to a placeholder still counts.
local function held(character: Character, item: number): number
    return p4.forge.vessel(character.id, item)
end
local function isVessel(item: number): boolean
    local row = rows[item]
    return not (row and row.kind == "signature")
end

-- The page is one state machine. Choices only record what was picked; the rows are
-- rebuilt at the next frame, because the page's own callbacks make no native calls.
type Step = "character" | "vessel" | "persona" | "confirm" | "result"
local step: Step = "character"
local chosen: {character: Character?, vessel: number, slot: number, persona: P4StockEntry?, row: Row?, fee: number} =
    {character = nil, vessel = 0, slot = 0, persona = nil, row = nil, fee = 0}
local view: {P4UIRow} = {}
local dirty = false
local committing = false
local equipping = false
local outcome = ""
-- The provider's last FORGED result, bound to the fighter it was forged for: "Equip now"
-- offers exactly this, and anything else that changes the page's flow clears it.
local forged: {character: Character, item: number}? = nil
-- Vessel counts for the character step, one fighter per frame: every fighter's vessel
-- list (retail weapons plus the forged rows) in one callback outruns the script budget.
-- shown is the step the page last built; entering the character step starts a recount.
local owned: {[number]: number} = {}
local counting = 0
local shown: Step? = nil

local function info(key: string, label: string, value: string?, help: string?): P4UIRow
    return {key = key, label = label, kind = "info", value = value, help = help}
end
local function action(key: string, label: string, value: string?, help: string?, disabled: boolean?): P4UIRow
    return {key = key, label = label, kind = "action", value = value, help = help, disabled = disabled}
end

local function build(): {P4UIRow}
    local out: {P4UIRow} = {}
    local character = chosen.character
    if step == "vessel" and character then
        for _, item in vessels[character.id] do
            local count = held(character, item)
            if count > 0 and isVessel(item) then
                table.insert(out, action("vessel:" .. item, itemName(item), "x" .. count,
                    "This weapon is consumed. Signature weapons cannot be vessels."))
            end
        end
        table.insert(out, action("back", "BACK"))
    elseif step == "persona" and character then
        local stock = p4.persona.stock()
        for _, entry in stock do
            local row, why = result(character, entry)
            local label = personaName(entry.persona)
            if entry.lead then
                table.insert(out, action("persona:" .. entry.slot, label, "EQUIPPED", "The equipped Persona cannot be fused.", true))
            elseif #stock < 2 then
                table.insert(out, action("persona:" .. entry.slot, label, "LAST", "The last Persona cannot be fused.", true))
            elseif row then
                local warning = if entry.registered then "" else " Not registered in the compendium."
                table.insert(out, action("persona:" .. entry.slot, label, row.name,
                    "Lv " .. entry.level .. ". Makes " .. row.name .. "." .. warning))
            else
                table.insert(out, action("persona:" .. entry.slot, label, "-", why, true))
            end
        end
        table.insert(out, action("back", "BACK"))
    elseif step == "confirm" and character then
        chosen.persona, chosen.row = nil, nil
        for _, entry in p4.persona.stock() do
            if entry.slot == chosen.slot and not entry.lead then
                chosen.persona = entry
                chosen.row = (result(character, entry))
            end
        end
        local row, persona = chosen.row, chosen.persona
        if not row or not persona then
            step = "persona"
            return build()
        end
        chosen.fee = math.floor(p4.data.item(row.anchor).price / 4)
        local money = p4.inventory.money()
        table.insert(out, info("who", "FOR", character.name))
        table.insert(out, info("vessel", "CONSUMES", itemName(chosen.vessel)))
        table.insert(out, info("persona", "AND", personaName(persona.persona)))
        table.insert(out, info("result", "MAKES", row.name))
        table.insert(out, info("fee", "FEE", chosen.fee .. " YEN", "You have " .. money .. " yen."))
        table.insert(out, action("forge", "FORGE", nil, "Fuse now. This cannot be undone.", money < chosen.fee))
        table.insert(out, action("back", "BACK"))
    elseif step == "result" then
        table.insert(out, info("outcome", outcome))
        local target = forged
        if target and not p4.equipment.pending() then
            local replaces, why = p4.equipment.check({character = target.character.id, slot = 0, item = target.item})
            local help = "Cannot equip it now: " .. tostring(why)
            if replaces == 0 then
                help = "Equip it."
            elseif replaces then
                help = "Equip it in place of " .. itemName(replaces) .. ", which returns to your items."
            end
            table.insert(out, action("equip", "EQUIP NOW", target.character.name, help, replaces == nil))
        end
        table.insert(out, action("done", "DONE"))
    else
        step = "character"
        if shown ~= "character" then owned, counting = {}, 1 end
        for _, c in CHARACTERS do
            local n = owned[c.id]
            table.insert(out, action("character:" .. c.id, c.name,
                if n == nil then "..." elseif n > 0 then n .. " VESSELS" else "NO VESSEL",
                "Choose whose weapon to forge. A vessel is an unequipped weapon of theirs.", n == nil or n == 0))
        end
    end
    shown = step
    return out
end

local page = p4.ui.page({
    key = "forge", title = "WEAPON FUSION",
    count = function() return #view end,
    row = function(index: number) return view[index] end,
    find = function(key: string)
        for i, row in view do if row.key == key then return i end end
        return nil
    end,
    adjust = function(key: string, direction: number)
        if direction ~= 0 or committing then return end
        local kind, value = string.match(key, "^(%a+):?(%d*)$")
        local n = tonumber(value) or 0
        if kind == "back" then
            step = if step == "confirm" then "persona" elseif step == "persona" then "vessel" else "character"
        elseif kind == "done" then
            forged = nil
            step = "character"
        elseif kind == "character" then
            for _, c in CHARACTERS do if c.id == n then chosen.character = c end end
            step = "vessel"
        elseif kind == "vessel" then
            chosen.vessel = n
            step = "persona"
        elseif kind == "persona" then
            chosen.slot = n
            step = "confirm"
        elseif kind == "forge" then
            forged = nil
            committing = true
        elseif kind == "equip" then
            equipping = forged ~= nil
        end
        dirty = true
    end,
    opened = function()
        forged = nil
        step = "character"
        shown = nil
        dirty = true
    end,
})

local command = p4.velvet.command({page = page, label = "WEAPON FUSION", operation = "weapon_fusion"})
command:disable()

-- The first dungeon's saved rescue-clear flag is the castle unlock.
local function unlocked(): boolean
    return p4.story.dungeon_cleared(1)
end
p4.events.on("field_enter", function()
    if unlocked() then command:enable() else command:disable() end
end)

p4.events.on("frame_start", function()
    if equipping then
        -- The provider validates fighter, slot, item and stock, then exchanges at the safe
        -- point the way the Camp equipment menu does: the old weapon returns to the items.
        equipping = false
        local target = forged
        if target then
            local ok, why = p4.equipment.equip({character = target.character.id, slot = 0, item = target.item})
            if not ok then outcome = "NOT EQUIPPED: " .. tostring(why) end
        end
        dirty = true
    end
    local worn = p4.equipment.result()
    if worn then
        local target = forged
        if worn.outcome == "equipped" then
            outcome = "EQUIPPED " .. itemName(worn.item)
            if target and target.character.id == worn.character and target.item == worn.item then forged = nil end
        else
            outcome = "NOT EQUIPPED: " .. tostring(worn.reason)
        end
        p4.log(string.format("FUSION_ARTS_EQUIP outcome=%s character=%d item=%d old=%d",
            worn.outcome, worn.character, worn.item, worn.previous))
        dirty = true
    end
    if committing then
        committing = false
        local character, persona, row = chosen.character, chosen.persona, chosen.row
        if character and persona and row then
            local ok, why = p4.forge.commit({character = character.id, persona = persona.persona,
                vessel = chosen.vessel, item = row.id, fee = chosen.fee, signature = row.kind == "signature"})
            if not ok then
                forged = nil
                outcome = "NOT FORGED: " .. tostring(why)
                step = "result"
            end
        end
        dirty = true
    end
    local done = p4.forge.result()
    if done then
        outcome = if done.outcome == "forged" then "FORGED " .. itemName(done.item) else "NOT FORGED: " .. tostring(done.reason)
        forged = nil
        if done.outcome == "forged" then
            for _, c in CHARACTERS do
                if c.id == done.character then forged = {character = c, item = done.item} end
            end
        end
        p4.log(string.format("FUSION_ARTS_FORGE outcome=%s character=%d persona=%d vessel=%d item=%d fee=%d",
            done.outcome, done.character, done.persona, done.vessel, done.item, done.fee))
        step = "result"
        dirty = true
    end
    if counting > 0 then
        if step ~= "character" then
            counting = 0
        else
            local c = CHARACTERS[counting]
            local n = 0
            for _, item in vessels[c.id] do
                if isVessel(item) and held(c, item) > 0 then n += 1 end
            end
            owned[c.id] = n
            counting = if counting < #CHARACTERS then counting + 1 else 0
            dirty = true
        end
    end
    if dirty then
        dirty = false
        view = build()
        page:refresh()
    end
end)

p4.log("FUSION_ARTS_READY weapon_fusion=on")
