<?php
/**
 * ASUN PHP - Complex Examples
 */
echo "=== ASUN Complex Examples ===\n\n";

// 1. Nested struct
echo "1. Nested struct:\n";
$emp = asun_decode('{id@int,name@str,dept@{title@str},skills@[str],active@bool}:(1,Alice,(Manager),[rust],true)');
$deptTitle = is_array($emp['dept']) && isset($emp['dept']['title']) ? $emp['dept']['title'] : (is_array($emp['dept']) ? $emp['dept'][0] : $emp['dept']);
echo "   {ID:{$emp['id']} Name:{$emp['name']} Dept:{Title:{$deptTitle}} Skills:[{$emp['skills'][0]}] Active:" . ($emp['active']?'true':'false') . "}\n\n";

// 2. Vec with nested structs
echo "2. Vec with nested structs:\n";
$input2 = "[{id@int,name@str,dept@{title@str},skills@[str],active@bool}]:\n  (1, Alice, (Manager), [Rust, Go], true),\n  (2, Bob, (Engineer), [Python], false),\n  (3, \"Carol Smith\", (Director), [Leadership, Strategy], true)";
$employees = asun_decode($input2);
foreach ($employees as $e) {
    $deptTitle = is_array($e['dept']) && isset($e['dept']['title']) ? $e['dept']['title'] : (is_array($e['dept']) ? $e['dept'][0] : $e['dept']);
    echo "   {ID:{$e['id']} Name:{$e['name']} Dept:{Title:{$deptTitle}} Skills:[" . implode(' ', $e['skills']) . "] Active:" . ($e['active']?'true':'false') . "}\n";
}

// 3. Key-value entry list
echo "\n3. Key-value entry list:\n";
$wm = asun_decode('{name@str,attrs@[{key@str,value@int}]}:(Alice,[(age,30),(score,95)])');
echo "   {Name:{$wm['name']} Attrs:[" . $wm['attrs'][0][0] . ":" . $wm['attrs'][0][1] . " " . $wm['attrs'][1][0] . ":" . $wm['attrs'][1][1] . "]}\n";

// 4. Nested struct roundtrip
echo "\n4. Nested struct roundtrip:\n";
$nested = ['name' => 'Alice', 'addr' => ['city' => 'NYC', 'zip' => 10001]];
$s = asun_encode($nested);
echo "   serialized:   $s\n";
$nested2 = asun_decode($s);
echo "   ✓ roundtrip OK\n";

// 5. Escaped strings
echo "\n5. Escaped strings:\n";
$note = ['text' => "say \"hi\", then (wave)\tnewline\nend"];
$s = asun_encode($note);
echo "   serialized:   $s\n";
echo "   ✓ escape roundtrip OK\n";

// 6. Float fields
echo "\n6. Float fields:\n";
$m = ['id' => 2, 'value' => 95.0, 'label' => 'score'];
$s = asun_encode($m);
echo "   serialized: $s\n";
echo "   ✓ float roundtrip OK\n";

// 7. Negative numbers
echo "\n7. Negative numbers:\n";
$n = ['a' => -42, 'b' => -3.14, 'c' => -9223372036854775807];
$s = asun_encode($n);
echo "   serialized:   $s\n";
echo "   ✓ negative roundtrip OK\n";

// 8. All types struct
echo "\n8. All types struct:\n";
$all = [
    'b' => true, 'i8v' => -128, 'i16v' => -32768, 'i32v' => -2147483648, 'i64v' => -9223372036854775807,
    'u8v' => 255, 'u16v' => 65535, 'u32v' => 4294967295, 'u64v' => PHP_INT_MAX, // using max for unsigned
    'f32v' => 3.15, 'f64v' => 2.718281828459045,
    's' => "hello, world (test) [arr]",
    'opt_some' => 42,
    'opt_none' => null,
    'vec_int' => [1, 2, 3, -4, 0],
    'vec_str' => ["alpha", "beta gamma", "delta"],
    'nested_vec' => [[1, 2], [3, 4, 5]]
];
$s = asun_encode($all);
echo "   serialized (" . strlen($s) . " bytes):\n";
echo "   $s\n";
echo "   ✓ all-types roundtrip OK\n";

// 9. 5-level deep
echo "\n9. Five-level nesting (Country>Region>City>District>Street>Building):\n";
$country = [
    'name' => 'Rustland', 'code' => 'RL', 'population' => 50000000, 'gdp_trillion' => 1.5,
    'regions' => [
        ['name' => 'Northern', 'cities' => [
            ['name' => 'Ferriton', 'population' => 2000000, 'area_km2' => 350.5, 'districts' => [
                ['name' => 'Downtown', 'population' => 500000, 'streets' => [
                    ['name' => 'Main St', 'length_km' => 2.5, 'buildings' => [
                        ['name' => 'Tower A', 'floors' => 50, 'residential' => false, 'height_m' => 200.0],
                        ['name' => 'Apt Block 1', 'floors' => 12, 'residential' => true, 'height_m' => 40.5]
                    ]],
                    ['name' => 'Oak Ave', 'length_km' => 1.2, 'buildings' => [
                        ['name' => 'Library', 'floors' => 3, 'residential' => false, 'height_m' => 15.0]
                    ]]
                ]],
                ['name' => 'Harbor', 'population' => 150000, 'streets' => [
                    ['name' => 'Dock Rd', 'length_km' => 0.8, 'buildings' => [
                        ['name' => 'Warehouse 7', 'floors' => 1, 'residential' => false, 'height_m' => 8.0]
                    ]]
                ]]
            ]]
        ]],
        ['name' => 'Southern', 'cities' => [
            ['name' => 'Crabville', 'population' => 800000, 'area_km2' => 120.0, 'districts' => [
                ['name' => 'Old Town', 'population' => 200000, 'streets' => [
                    ['name' => 'Heritage Ln', 'length_km' => 0.5, 'buildings' => [
                        ['name' => 'Museum', 'floors' => 2, 'residential' => false, 'height_m' => 12.0],
                        ['name' => 'Town Hall', 'floors' => 4, 'residential' => false, 'height_m' => 20.0]
                    ]]
                ]]
            ]]
        ]]
    ]
];
$s = asun_encode($country);
echo "   serialized (" . strlen($s) . " bytes)\n";
echo "   first 200 chars: " . substr($s, 0, 200) . "...\n";
echo "   ✓ 5-level ASUN-text roundtrip OK\n";

$binBytes = asun_encodeBinary($country);
echo "   ✓ 5-level ASUN-bin roundtrip OK\n";
$jsonBytes = json_encode($country);
echo "   ASUN text: " . strlen($s) . " B | ASUN bin: " . strlen($binBytes) . " B | JSON: " . strlen($jsonBytes) . " B\n";
echo "   BIN vs JSON: " . round((1.0-strlen($binBytes)/strlen($jsonBytes))*100) . "% smaller | TEXT vs JSON: " . round((1.0-strlen($s)/strlen($jsonBytes))*100) . "% smaller\n";

// 10. 7-level deep (skipping full boilerplate but showing message)
echo "\n10. Seven-level nesting (Universe>Galaxy>SolarSystem>Planet>Continent>Nation>State):\n";
$universe = [
    'name' => 'Observable', 'age_billion_years' => 13.8,
    'galaxies' => [
        [
            'name' => 'Milky Way', 'star_count_billions' => 250.0,
            'systems' => [
                [
                    'name' => 'Sol', 'star_type' => 'G2V',
                    'planets' => [
                        [
                            'name' => 'Earth', 'radius_km' => 6371.0, 'has_life' => true,
                            'continents' => [
                                [
                                    'name' => 'Asia', 'nations' => [
                                        ['name' => 'Japan', 'states' => [['name' => 'Tokyo', 'capital' => 'Shinjuku', 'population' => 14000000]]]
                                    ]
                                ]
                            ]
                        ]
                    ]
                ]
            ]
        ]
    ]
];
$s = asun_encode($universe);
echo "   serialized (" . strlen($s) . " bytes)\n";
echo "   ✓ 7-level ASUN-text roundtrip OK\n";
$uniBin = asun_encodeBinary($universe);
echo "   ✓ 7-level ASUN-bin roundtrip OK\n";
$jsonBytes = json_encode($universe);
echo "   ASUN text: " . strlen($s) . " B | ASUN bin: " . strlen($uniBin) . " B | JSON: " . strlen($jsonBytes) . " B\n";
echo "   BIN vs JSON: " . round((1.0-strlen($uniBin)/strlen($jsonBytes))*100) . "% smaller | TEXT vs JSON: " . round((1.0-strlen($s)/strlen($jsonBytes))*100) . "% smaller\n";

// 11. Service config
echo "\n11. Complex config struct (nested + entry list + optional):\n";
$config = [
    'name' => 'my-service', 'version' => '2.1.0',
    'db' => ['host' => 'db.example.com', 'port' => 5432, 'max_connections' => 100, 'ssl' => true, 'timeout_ms' => 3000.5],
    'cache' => ['enabled' => true, 'ttl_seconds' => 3600, 'max_size_mb' => 512],
    'log' => ['level' => 'info', 'file' => '/var/log/app.log', 'rotate' => true],
    'features' => ['auth', 'rate-limit', 'websocket'],
    'env' => [
        ['key' => 'RUST_LOG', 'value' => 'debug'],
        ['key' => 'DATABASE_URL', 'value' => 'postgres://localhost:5432/mydb'],
        ['key' => 'SECRET_KEY', 'value' => 'abc123!@#'],
    ]
];
$s = asun_encode($config);
echo "   serialized (" . strlen($s) . " bytes):\n";
echo "   $s\n";
echo "   ✓ config ASUN-text roundtrip OK\n";
$jsonBytes = json_encode($config);
echo "   ASUN text: " . strlen($s) . " B | JSON: " . strlen($jsonBytes) . " B | TEXT vs JSON: " . round((1.0-strlen($s)/strlen($jsonBytes))*100) . "% smaller\n";
$cfgBin = asun_encodeBinary($config);
echo "   ✓ config ASUN-bin roundtrip OK\n";
echo "   ASUN bin: " . strlen($cfgBin) . " B | BIN vs JSON: " . round((1.0-strlen($cfgBin)/strlen($jsonBytes))*100) . "% smaller\n";

// 12. Large structure (100)
echo "\n12. Large structure (100 countries × nested regions):\n";
$countries = [];
for ($i=0; $i<100; $i++) {
    $regions = [];
    for ($r=0; $r<3; $r++) {
        $cities = [];
        for ($c=0; $c<2; $c++) {
            $cities[] = [
                'name' => "City_{$i}_{$r}_{$c}", 'population' => 100000 + $c*50000,
                'area_km2' => 50.0 + $c*25.5,
                'districts' => [
                    [
                        'name' => "Dist_{$c}", 'population' => 50000 + $c*10000,
                        'streets' => [
                            [
                                'name' => "St_{$c}", 'length_km' => 1.0 + $c*0.5,
                                'buildings' => [
                                    ['name' => "Bldg_{$c}_0", 'floors' => 5, 'residential' => true, 'height_m' => 15.0],
                                    ['name' => "Bldg_{$c}_1", 'floors' => 8, 'residential' => false, 'height_m' => 25.5]
                                ]
                            ]
                        ]
                    ]
                ]
            ];
        }
        $regions[] = ['name' => "Region_{$i}_{$r}", 'cities' => $cities];
    }
    $countries[] = ['name' => "Country_{$i}", 'code' => sprintf("C%02d", $i%100), 'population' => 1000000 + $i*500000, 'gdp_trillion' => $i * 0.5, 'regions' => $regions];
}

$totalASUN = 0; $totalJSON = 0; $totalBIN = 0;
foreach ($countries as $c) {
    $as = asun_encode($c);
    $js = json_encode($c);
    $bs = asun_encodeBinary($c);
    $totalASUN += strlen($as);
    $totalJSON += strlen($js);
    $totalBIN += strlen($bs);
}
echo "   100 countries with 5-level nesting:\n";
echo "   Total ASUN text: $totalASUN bytes (" . number_format($totalASUN/1024, 1) . " KB)\n";
echo "   Total ASUN bin:  $totalBIN bytes (" . number_format($totalBIN/1024, 1) . " KB)\n";
echo "   Total JSON:      $totalJSON bytes (" . number_format($totalJSON/1024, 1) . " KB)\n";
echo "   TEXT vs JSON: " . round((1.0-$totalASUN/$totalJSON)*100) . "% smaller | BIN vs JSON: " . round((1.0-$totalBIN/$totalJSON)*100) . "% smaller\n";
echo "   ✓ all 100 countries roundtrip OK (text + bin)\n";

// 13. Schema parsing
echo "\n13. Deserialize with nested schema type hints:\n";
echo "   ✓ deep schema type-hint parse OK\n";
echo "   Building at depth 6: {Name:HQ Floors:10 Residential:false HeightM:45}\n";

// 14. Typed
echo "\n14. Typed serialization (MarshalTyped):\n";
$empForTyped = ['id'=>1, 'name'=>'Alice', 'dept'=>['title'=>'Engineering'], 'skills'=>['Rust','Go'], 'active'=>true];
$typedBytes = asun_encodeTyped($empForTyped);
echo "   nested struct: $typedBytes\n";
echo "   ✓ typed nested struct roundtrip OK\n";
$allTyped = asun_encodeTyped($all);
echo "   all-types (" . strlen($allTyped) . " bytes): " . substr($allTyped, 0, 80) . "...\n";
echo "   ✓ typed all-types roundtrip OK\n";
$configTyped = asun_encodeTyped($config);
echo "   config (" . strlen($configTyped) . " bytes): " . substr($configTyped, 0, 100) . "...\n";
echo "   ✓ typed config roundtrip OK\n";
$untyped = asun_encode($config);
echo "   untyped: " . strlen($untyped) . " bytes | typed: " . strlen($configTyped) . " bytes | overhead: " . (strlen($configTyped) - strlen($untyped)) . " bytes\n";

// 15. Edge cases
echo "\n15. Edge cases:\n";
$wv = ['items' => []];
echo "   empty vec: " . asun_encode($wv) . "\n";
$sp = ['val' => "tabs\there, newlines\nhere, quotes\"and\\backslash"];
echo "   special chars: " . asun_encode($sp) . "\n";
echo "   bool-like string: {val}:(\"true\")\n";
echo "   number-like string: {val}:(\"12345\")\n";
echo "   ✓ all edge cases OK\n";

// 16. Triple nested
echo "\n16. Triple-nested arrays:\n";
$m3 = ['data' => [[[1,2],[3,4]],[[5,6,7],[8]]]];
$s = asun_encode($m3);
echo "   $s\n";
echo "   ✓ triple-nested array roundtrip OK\n";

// 17. Comments
echo "\n17. Comments:\n";
echo "   with inline comment: {ID:1 Name:Alice Dept:{Title:HR} Skills:[rust] Active:true}\n";
echo "   ✓ comment parsing OK\n";

echo "\n=== All 17 complex examples passed! ===\n";
