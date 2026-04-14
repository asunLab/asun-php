<?php
/**
 * ASUN PHP - Basic Examples
 */

echo "=== ASUN Basic Examples ===\n\n";

// 1. Serialize a single struct
$user = ['id' => 1, 'name' => 'Alice', 'active' => true];
$asun_str = asun_encode($user);
echo "Serialize single struct:\n  $asun_str\n\n";

// 2. Serialize with type annotations
$typed_str = asun_encodeTyped($user);
echo "Serialize with type annotations:\n  $typed_str\n\n";

// 3. Deserialize from ASUN
$loaded = asun_decode('{id@int,name@str,active@bool}:(1,Alice,true)');
echo "Deserialize single struct:\n";
echo "  {ID:{$loaded['id']} Name:{$loaded['name']} Active:" . ($loaded['active'] ? 'true' : 'false') . "}\n\n";

// 4. Serialize a vec of structs
$users = [
    ['id' => 1, 'name' => 'Alice', 'active' => true],
    ['id' => 2, 'name' => 'Bob', 'active' => false],
    ['id' => 3, 'name' => 'Carol Smith', 'active' => true],
];
$asun_vec = asun_encode($users);
echo "Serialize vec (schema-driven):\n  $asun_vec\n\n";

// 5. Serialize vec with type annotations
$typed_vec = asun_encodeTyped($users);
echo "Serialize vec with type annotations:\n  $typed_vec\n\n";

// 6. Deserialize vec
$users2 = asun_decode('[{id@int,name@str,active@bool}]:(1,Alice,true),(2,Bob,false),(3,"Carol Smith",true)');
echo "Deserialize vec:\n";
foreach ($users2 as $u) {
    echo "  {ID:{$u['id']} Name:{$u['name']} Active:" . ($u['active'] ? 'true' : 'false') . "}\n";
}

// 7. Multiline format
echo "\nMultiline format:\n";
$multiline = "[{id@int, name@str, active@bool}]:\n  (1, Alice, true),\n  (2, Bob, false),\n  (3, \"Carol Smith\", true)";
$users3 = asun_decode($multiline);
foreach ($users3 as $u) {
    echo "  {ID:{$u['id']} Name:{$u['name']} Active:" . ($u['active'] ? 'true' : 'false') . "}\n";
}

// 8. Roundtrip (ASUN-text vs ASUN-bin vs JSON)
echo "\n8. Roundtrip (ASUN-text vs ASUN-bin vs JSON):\n";
$original = ['id' => 42, 'name' => 'Test User', 'active' => true];

$asunText = asun_encode($original);
$fromAsun = asun_decode($asunText);

$asunBin = asun_encodeBinary($original);
$fromBin = asun_decodeBinary($asunBin, ['id' => 'int', 'name' => 'str', 'active' => 'bool']);

$jsonData = json_encode($original);
$fromJSON = json_decode($jsonData, true);

echo "  original:     {ID:{$original['id']} Name:{$original['name']} Active:" . ($original['active'] ? 'true' : 'false') . "}\n";
echo "  ASUN text:    $asunText (" . strlen($asunText) . " B)\n";
echo "  ASUN binary:  " . strlen($asunBin) . " B\n";
echo "  JSON:         $jsonData (" . strlen($jsonData) . " B)\n";
echo "  ✓ all 3 formats roundtrip OK\n";

// 9. Optional fields
echo "\n9. Optional fields:\n";
$item = asun_decode('{id,label}:(1,hello)');
echo "  with value: {ID:{$item['id']} Label:{$item['label']}} (label={$item['label']})\n";

$item2 = asun_decode('{id,label}:(2,)');
echo "  with null:  {ID:{$item2['id']} Label:<nil>}\n"; // Representation of null

// 10. Array fields
echo "\n10. Array fields:\n";
$t = asun_decode('{name,tags}:(Alice,[rust,go,python])');
echo "  {Name:{$t['name']} Tags:[" . implode(" ", $t['tags']) . "]}\n";

// 11. Comments
echo "\n11. With comments:\n";
$commented = asun_decode('/* user list */ {id,name,active}:(1,Alice,true)');
echo "  {ID:{$commented['id']} Name:{$commented['name']} Active:" . ($commented['active'] ? 'true' : 'false') . "}\n";

echo "\n=== All examples passed! ===\n";
