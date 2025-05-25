<?php
$data = file_get_contents("php://input");
if ($data) {
    file_put_contents("cam.jpg", $data);
    echo "Imagem salva com sucesso.";
} else {
    echo "Nenhum dado recebido.";
}
?>
