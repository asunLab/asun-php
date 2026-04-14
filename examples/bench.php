<?php
/**
 * ASUN PHP benchmark.
 *
 * Output style matches the repository's JSON / ASUN / BIN benchmark format.
 */

function generateUsers(int $n): array {
    $names = ["Alice", "Bob", "Carol", "David", "Eve", "Frank", "Grace", "Hank"];
    $roles = ["engineer", "designer", "manager", "analyst"];
    $cities = ["NYC", "LA", "Chicago", "Houston", "Phoenix"];
    $users = [];
    for ($i = 0; $i < $n; $i++) {
        $users[] = [
            'id' => $i,
            'name' => $names[$i % count($names)],
            'email' => strtolower($names[$i % count($names)]) . "@example.com",
            'age' => 25 + ($i % 40),
            'score' => 50.0 + ($i % 50) + 0.5,
            'active' => ($i % 3 !== 0),
            'role' => $roles[$i % count($roles)],
            'city' => $cities[$i % count($cities)],
        ];
    }
    return $users;
}

function generateAllTypes(int $n): array {
    $items = [];
    for ($i = 0; $i < $n; $i++) {
        $items[] = [
            'b' => ($i % 2 === 0),
            'iv' => -($i * 100_000),
            'pv' => $i * 1_000_000 + 7,
            'fv' => $i * 0.25 + 0.5,
            'sv' => "item_$i",
            'count' => $i + 10,
            'tag' => "tag_" . ($i % 5),
        ];
    }
    return $items;
}

function generateCompanies(int $n): array {
    $locs = ["NYC", "London", "Tokyo", "Berlin"];
    $leads = ["Alice", "Bob", "Carol", "David"];
    $companies = [];
    for ($i = 0; $i < $n; $i++) {
        $divisions = [];
        for ($d = 0; $d < 2; $d++) {
            $teams = [];
            for ($t = 0; $t < 2; $t++) {
                $projects = [];
                for ($p = 0; $p < 3; $p++) {
                    $tasks = [];
                    for ($tk = 0; $tk < 4; $tk++) {
                        $tasks[] = [
                            'id' => $i * 100 + $d * 10 + $t * 5 + $tk,
                            'title' => "Task_$tk",
                            'priority' => ($tk % 3) + 1,
                            'done' => ($tk % 2) === 0,
                            'hours' => 2.0 + $tk * 1.5,
                        ];
                    }
                    $projects[] = [
                        'name' => "Proj_{$t}_{$p}",
                        'budget' => 100.0 + $p * 50.5,
                        'active' => ($p % 2) === 0,
                        'tasks' => $tasks,
                    ];
                }
                $teams[] = [
                    'name' => "Team_{$i}_{$d}_{$t}",
                    'lead' => $leads[$t % count($leads)],
                    'size' => 5 + $t * 2,
                    'projects' => $projects,
                ];
            }
            $divisions[] = [
                'name' => "Div_{$i}_{$d}",
                'location' => $locs[$d % count($locs)],
                'headcount' => 50 + $d * 20,
                'teams' => $teams,
            ];
        }
        $companies[] = [
            'name' => "Corp_$i",
            'founded' => 1990 + ($i % 35),
            'revenue_m' => 10.0 + $i * 5.5,
            'public' => ($i % 2) === 0,
            'divisions' => $divisions,
            'tags' => ["enterprise", "tech", "sector_" . ($i % 5)],
        ];
    }
    return $companies;
}

function flatSchema(): array {
    return [
        'id' => 'int',
        'name' => 'str',
        'email' => 'str',
        'age' => 'int',
        'score' => 'float',
        'active' => 'bool',
        'role' => 'str',
        'city' => 'str',
    ];
}

function allTypesSchema(): array {
    return [
        'b' => 'bool',
        'iv' => 'int',
        'pv' => 'int',
        'fv' => 'float',
        'sv' => 'str',
        'count' => 'int',
        'tag' => 'str',
    ];
}

function taskSchema(): array {
    return [
        'id' => 'int',
        'title' => 'str',
        'priority' => 'int',
        'done' => 'bool',
        'hours' => 'float',
    ];
}

function projectSchema(): array {
    return [
        'name' => 'str',
        'budget' => 'float',
        'active' => 'bool',
        'tasks' => [taskSchema()],
    ];
}

function teamSchema(): array {
    return [
        'name' => 'str',
        'lead' => 'str',
        'size' => 'int',
        'projects' => [projectSchema()],
    ];
}

function divisionSchema(): array {
    return [
        'name' => 'str',
        'location' => 'str',
        'headcount' => 'int',
        'teams' => [teamSchema()],
    ];
}

function companySchema(): array {
    return [
        'name' => 'str',
        'founded' => 'int',
        'revenue_m' => 'float',
        'public' => 'bool',
        'divisions' => [divisionSchema()],
        'tags' => ['str'],
    ];
}

function bench(callable $fn, int $iters): float {
    $start = hrtime(true);
    for ($i = 0; $i < $iters; $i++) {
        $fn();
    }
    return (hrtime(true) - $start) / 1e6;
}

function formatRatio(float $baseMs, float $targetMs): string {
    if ($targetMs <= 0.0) {
        return "infx";
    }
    $ratio = rtrim(rtrim(number_format($baseMs / $targetMs, 1, '.', ''), '0'), '.');
    return $ratio . "x";
}

function formatPercent(int $part, int $whole): string {
    if ($whole <= 0) {
        return "0%";
    }
    $pct = rtrim(rtrim(number_format($part * 100.0 / $whole, 1, '.', ''), '0'), '.');
    return $pct . "%";
}

function printSection(string $title, int $width = 68): void {
    $line = str_repeat("─", $width - 2);
    echo "┌{$line}┐\n";
    printf("│ %-".($width - 4)."s │\n", $title);
    echo "└{$line}┘\n";
}

function printResult(
    string $name,
    float $jsonSerMs,
    float $asunSerMs,
    float $binSerMs,
    float $jsonDeMs,
    float $asunDeMs,
    float $binDeMs,
    int $jsonBytes,
    int $asunBytes,
    int $binBytes
): void {
    echo "  {$name}\n";
    printf(
        "    Serialize:   JSON %8.2fms/%dB | ASUN %8.2fms(%s)/%dB(%s) | BIN %8.2fms(%s)/%dB(%s)\n",
        $jsonSerMs,
        $jsonBytes,
        $asunSerMs,
        formatRatio($jsonSerMs, $asunSerMs),
        $asunBytes,
        formatPercent($asunBytes, $jsonBytes),
        $binSerMs,
        formatRatio($jsonSerMs, $binSerMs),
        $binBytes,
        formatPercent($binBytes, $jsonBytes)
    );
    printf(
        "    Deserialize: JSON %8.2fms | ASUN %8.2fms(%s) | BIN %8.2fms(%s)\n",
        $jsonDeMs,
        $asunDeMs,
        formatRatio($jsonDeMs, $asunDeMs),
        $binDeMs,
        formatRatio($jsonDeMs, $binDeMs)
    );
}

function runCase(string $name, mixed $value, array $binarySchema, int $iterations): void {
    $jsonData = json_encode($value, JSON_UNESCAPED_UNICODE);
    $asunData = asun_encodeTyped($value);
    $binData = asun_encodeBinary($value);

    $jsonSerMs = bench(static fn() => json_encode($value, JSON_UNESCAPED_UNICODE), $iterations);
    $asunSerMs = bench(static fn() => asun_encodeTyped($value), $iterations);
    $binSerMs = bench(static fn() => asun_encodeBinary($value), $iterations);

    $jsonDeMs = bench(static fn() => json_decode($jsonData, true), $iterations);
    $asunDeMs = bench(static fn() => asun_decode($asunData), $iterations);
    $binDeMs = bench(static fn() => asun_decodeBinary($binData, $binarySchema), $iterations);

    printResult(
        $name,
        $jsonSerMs,
        $asunSerMs,
        $binSerMs,
        $jsonDeMs,
        $asunDeMs,
        $binDeMs,
        strlen($jsonData),
        strlen($asunData),
        strlen($binData)
    );
}

function runFlatSection(): void {
    printSection("Section 1: Flat Struct (8 fields)");
    foreach ([[100, 100], [500, 100], [1000, 100], [5000, 20]] as [$count, $iters]) {
        runCase("Flat struct × {$count} (8 fields, vec)", generateUsers($count), [flatSchema()], $iters);
        echo "\n";
    }
}

function runAllTypesSection(): void {
    printSection("Section 2: All-Types Struct (7 fields)");
    foreach ([[100, 100], [500, 100]] as [$count, $iters]) {
        runCase("All-types struct × {$count} (7 fields)", generateAllTypes($count), [allTypesSchema()], $iters);
        echo "\n";
    }
}

function runDeepSection(): void {
    printSection("Section 3: 5-Level Deep Nesting");
    foreach ([[10, 50], [50, 20], [100, 10]] as [$count, $iters]) {
        runCase("5-level deep × {$count} (Company hierarchy)", generateCompanies($count), [companySchema()], $iters);
        echo "\n";
    }
}

function runSingleSection(): void {
    printSection("Section 4: Single Struct Roundtrip");
    runCase("Single flat struct × 10000 (8 fields)", generateUsers(1)[0], flatSchema(), 10000);
    echo "\n";
    runCase("Single deep company × 1000", generateCompanies(1)[0], companySchema(), 1000);
    echo "\n";
}

function runLargePayloadSection(): void {
    printSection("Section 5: Large Payload (10k records)");
    runCase("Large payload × 10000 (8 fields, vec)", generateUsers(10000), [flatSchema()], 10);
    echo "\n";
}

function runThroughputSection(): void {
    printSection("Section 6: Throughput Summary");
    $rows = generateUsers(1000);
    $jsonData = json_encode($rows, JSON_UNESCAPED_UNICODE);
    $asunData = asun_encodeTyped($rows);
    $binData = asun_encodeBinary($rows);
    $iters = 100;
    $totalRecords = count($rows) * $iters;

    $jsonSerMs = bench(static fn() => json_encode($rows, JSON_UNESCAPED_UNICODE), $iters);
    $asunSerMs = bench(static fn() => asun_encodeTyped($rows), $iters);
    $binSerMs = bench(static fn() => asun_encodeBinary($rows), $iters);
    $jsonDeMs = bench(static fn() => json_decode($jsonData, true), $iters);
    $asunDeMs = bench(static fn() => asun_decode($asunData), $iters);
    $binDeMs = bench(static fn() => asun_decodeBinary($binData, [flatSchema()]), $iters);

    $jsonSerRps = $totalRecords / ($jsonSerMs / 1000.0);
    $asunSerRps = $totalRecords / ($asunSerMs / 1000.0);
    $binSerRps = $totalRecords / ($binSerMs / 1000.0);
    $jsonDeRps = $totalRecords / ($jsonDeMs / 1000.0);
    $asunDeRps = $totalRecords / ($asunDeMs / 1000.0);
    $binDeRps = $totalRecords / ($binDeMs / 1000.0);

    printf(
        "  Serialize throughput:   JSON %12s rec/s | ASUN %12s rec/s (%s) | BIN %12s rec/s (%s)\n",
        number_format($jsonSerRps, 0, '.', ','),
        number_format($asunSerRps, 0, '.', ','),
        formatRatio($asunSerRps, $jsonSerRps),
        number_format($binSerRps, 0, '.', ','),
        formatRatio($binSerRps, $jsonSerRps)
    );
    printf(
        "  Deserialize throughput: JSON %12s rec/s | ASUN %12s rec/s (%s) | BIN %12s rec/s (%s)\n",
        number_format($jsonDeRps, 0, '.', ','),
        number_format($asunDeRps, 0, '.', ','),
        formatRatio($asunDeRps, $jsonDeRps),
        number_format($binDeRps, 0, '.', ','),
        formatRatio($binDeRps, $jsonDeRps)
    );
    printf("  Size baseline (1k rows): JSON %dB | ASUN %dB(%s) | BIN %dB(%s)\n\n", strlen($jsonData), strlen($asunData), formatPercent(strlen($asunData), strlen($jsonData)), strlen($binData), formatPercent(strlen($binData), strlen($jsonData)));
}

echo "╔══════════════════════════════════════════════════════════════╗\n";
echo "║                ASUN PHP vs JSON Benchmark                    ║\n";
echo "╚══════════════════════════════════════════════════════════════╝\n\n";
echo "System: " . php_uname('s') . " " . php_uname('m') . " | PHP " . PHP_VERSION . "\n";
echo "Mode: compact JSON vs typed ASUN text vs ASUN binary\n";
echo "Scope: structs, nested arrays, entry-object lists\n\n";

runFlatSection();
runAllTypesSection();
runDeepSection();
runSingleSection();
runLargePayloadSection();
runThroughputSection();

echo "╔══════════════════════════════════════════════════════════════╗\n";
echo "║                    Benchmark Complete                        ║\n";
echo "╚══════════════════════════════════════════════════════════════╝\n";
