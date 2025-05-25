<?php
// arquivo: status.php  (ou você pode sobrescrever dados_sensores.php)

// 1) Lê o último sensor
$sensorFile = __DIR__ . '/dados_sensores.txt';
$lastLine   = '';
if (file_exists($sensorFile)) {
    $lines = file($sensorFile, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
    if (!empty($lines)) {
        $lastLine = end($lines);
    }
}

// Decodifica JSON do sensor
$data = json_decode($lastLine, true);
if (!is_array($data)) {
    $data = [
        'distancia' => null,
        'timestamp' => null,
        'alert'     => null
    ];
} else {
    // garante as chaves
    foreach (['distancia','timestamp','alert'] as $k) {
        if (!array_key_exists($k, $data)) {
            $data[$k] = null;
        }
    }
}

// 2) Lê o comando atual
$cmdFile = __DIR__ . '/comando.txt';
$cmd = null;
if (file_exists($cmdFile)) {
    $c = trim(file_get_contents($cmdFile));
    if ($c !== '') {
        $cmd = $c;
    }
}
// adiciona ao array
$data['comando'] = $cmd;

// 3) Retorna JSON
header('Content-Type: application/json');
echo json_encode($data, JSON_UNESCAPED_SLASHES);
