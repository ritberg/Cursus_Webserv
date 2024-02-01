<?php
// HTML content
echo "<html><head><title>Photo Upload CGI</title></head><body>";
echo "<h1>Photo uploading</h1>";

// Retrieve user input from the form
if ($_SERVER['REQUEST_METHOD'] == 'POST') {
    echo "<form action='' method='post' enctype='multipart/form-data'>";
    echo "  <label for='name'>Your Name:</label>";
    echo "  <input type='text' name='name'>";
    echo "  <br>";
    echo "  <label for='photo'>Choose a photo:</label>";
    echo "  <input type='file' name='photo'>";
    echo "  <br>";
    echo "  <input type='submit' value='Upload'>";
    echo "</form>";
}

echo "</body></html>";
?>

