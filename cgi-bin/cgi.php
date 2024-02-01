<?php
// HTML content
echo "<html><head><title>Photo Upload CGI</title></head><body>";
echo "<h1>Photo uploading</h1>";

// Check if the form has been submitted
if ($_SERVER['REQUEST_METHOD'] == 'POST') {
    // Retrieve user input from the form
    echo "<form action='' method='post' enctype='multipart/form-data'>";
    echo "  <label for='name'>Your Name:</label>";
    echo "  <input type='text' name='name'>";
    echo "  <br>";
    echo "  <label for='photo'>Choose a photo:</label>";
    echo "  <input type='file' name='photo'>";
    echo "  <br>";
    echo "  <input type='submit' value='Upload'>";
    echo "</form>";

    // Display additional buttons for download and delete
    echo "<form action='' method='post'>";
    echo "  <input type='hidden' name='action' value='download'>";
    echo "  <input type='submit' value='Download'>";
    echo "</form>";

    echo "<form action='' method='post'>";
    echo "  <input type='hidden' name='action' value='delete'>";
    echo "  <input type='submit' value='Delete'>";
    echo "</form>";
}

echo "</body></html>";
?>
