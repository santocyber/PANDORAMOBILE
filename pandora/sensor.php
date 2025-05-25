<?php
// sensor.php

// 1) Só aceita POST
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    header('Allow: POST');
    header('Content-Type: application/json');
    echo json_encode(['error' => 'Method Not Allowed, use POST']);
    exit;
}

// 2) Lê e decodifica o JSON
$raw = file_get_contents('php://input');
$data = json_decode($raw, true);

if (json_last_error() !== JSON_ERROR_NONE || !is_array($data)) {
    http_response_code(400);
    header('Content-Type: application/json');
    echo json_encode(['error' => 'Invalid JSON']);
    exit;
}

// 3) Normaliza os campos que vamos guardar
$entry = [
    'distancia' => array_key_exists('distancia', $data) ? $data['distancia'] : null,
    'timestamp' => array_key_exists('timestamp', $data)
                   ? $data['timestamp']
                   : date('c'),   // ISO-8601 se não vier
    'alert'     => array_key_exists('alert', $data) ? $data['alert'] : null
];

// 4) Anexa ao arquivo (uma linha = um JSON)
file_put_contents(
    __DIR__ . '/dados_sensores.txt',
    json_encode($entry, JSON_UNESCAPED_SLASHES) . PHP_EOL,
    FILE_APPEND
);

// 5) Responde OK em JSON
header('Content-Type: application/json');
echo json_encode(['status' => 'ok']);
