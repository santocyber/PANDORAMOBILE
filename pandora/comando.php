<?php
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['cmd'])) {
    file_put_contents('comando.txt', $_POST['cmd']);
}
echo file_get_contents('comando.txt');
?>
