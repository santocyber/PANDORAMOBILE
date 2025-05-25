<!DOCTYPE html>
<html lang="pt-br" data-bs-theme="dark">
<head>
  <meta charset="UTF-8">
  <title>Controle do Robô</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">

  <!-- Bootstrap 5 -->
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/bootstrap.min.css" rel="stylesheet">

  <style>
    #camera-feed {
      width: 100%;
      max-width: 480px;
      border: 2px solid #555;
      border-radius: 5px;
    }
    .btn-group {
      margin-top: 15px;
      flex-wrap: wrap;
    }
    .btn-group .btn {
      margin: 5px;
      min-width: 120px;
    }
    .theme-switcher {
      position: fixed;
      top: 1rem;
      right: 1rem;
    }
  </style>
</head>
<body class="text-center p-4 bg-body-tertiary">

  <!-- Theme switch -->
  <div class="theme-switcher">
    <div class="form-check form-switch text-end">
      <input class="form-check-input" type="checkbox" id="themeToggle" checked>
      <label class="form-check-label" for="themeToggle">🌙</label>
    </div>
  </div>

  <div class="container">
    <h1 class="mb-4">🤖 Controle do Robô</h1>

    <h4 class="mb-3">Imagem da Câmera</h4>
    <img id="camera-feed" src="cam.jpg?ts=<?= time() ?>" alt="Imagem da Câmera" class="img-fluid mb-4">



<h4 class="mb-3">Sensores</h4>
<div class="card text-start mx-auto mb-4" style="max-width: 480px;" id="sensor-card">
  <div class="card-body">
    <h5 class="card-title">Ultrassônico</h5>
    <p class="card-text">
      Distância: <span id="distancia">--</span> cm<br>
      Timestamp: <span id="sensor-time">--</span><br>
      Status: <span id="alert">--</span><br>
      Comando: <span id="comando">--</span><br>
    </p>
  </div>
</div>


<h4 class="mb-3">Comandos</h4>
<div class="d-flex justify-content-center btn-group flex-wrap" id="command-buttons">
  <?php
  $comandos = [
    // movimento básico
    "f"        => "Frente (f)",
    "b"        => "Ré (b)",
    "s"        => "Parar (s)",
    // giro no próprio eixo
    "c"        => "Girar CCW (c)",
    "w"        => "Girar CW (w)",

    // posicionamento absoluto do servo
    "A180"     => "Servo ↞ Esquerda (A180)",
    "A90"      => "Servo • Centro (A90)",
    "A0"       => "Servo ↠ Direita (A0)",
    // flash do ESP32
    "flash_on" => "Flash On",
    "flash_off"=> "Flash Off"
  ];

  foreach ($comandos as $cmd => $label) {
    echo "<button 
            class='btn btn-primary m-1' 
            onclick=\"enviarComando('{$cmd}')\">
            {$label}
          </button>";
  }
  ?>
</div>

<div id="status" class="mt-3 text-center"></div>
  <script>
    // Atualiza imagem da câmera a cada 1.5 segundos
    setInterval(() => {
      const img = document.getElementById('camera-feed');
      img.src = 'cam.jpg?ts=' + new Date().getTime();
    }, 1500);

    // Envia comando via POST sem recarregar a página
    function enviarComando(cmd) {
      fetch('comando.php', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'cmd=' + encodeURIComponent(cmd)
      })
      .then(response => response.text())
      .then(text => {
        document.getElementById('status').innerHTML =
          `<div class="alert alert-success alert-dismissible fade show" role="alert">
            Comando <strong>${cmd}</strong> enviado com sucesso.
            <button type="button" class="btn-close" data-bs-dismiss="alert" aria-label="Close"></button>
          </div>`;
      })
      .catch(err => {
        document.getElementById('status').innerHTML =
          `<div class="alert alert-danger">Erro ao enviar comando: ${err}</div>`;
      });
    }

    // Alternância entre modo claro e escuro
    document.getElementById('themeToggle').addEventListener('change', function () {
      document.documentElement.setAttribute('data-bs-theme', this.checked ? 'dark' : 'light');
      this.nextElementSibling.textContent = this.checked ? '🌙' : '☀️';
    });
    
    
    
    
    // Atualiza dados do sensor a cada 2 segundos
setInterval(() => {
  fetch('ultimos_sensores.php')
    .then(res => res.json())
    .then(data => {
      document.getElementById('distancia').innerText = data.distancia ?? '--';
      document.getElementById('sensor-time').innerText = data.timestamp ?? '--';
      document.getElementById('alert').innerText = data.alert ?? '--';
      document.getElementById('comando').innerText = data.comando ?? '--';
    })
    .catch(err => {
      console.error("Erro ao buscar sensor:", err);
    });
}, 2000);



  </script>

  <!-- Bootstrap JS (opcional, para fechar alertas) -->
  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/js/bootstrap.bundle.min.js"></script>
</body>
</html>
