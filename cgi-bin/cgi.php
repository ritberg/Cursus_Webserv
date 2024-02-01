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

    if (isset($_FILES['photo'])) {             // Success msg TO DO
        echo "<p>Successfully uploaded!</p>";
    }

    // Display additional button for delete
    echo "<form action='' method='post' id='deleteForm'>";
    echo "  <input type='hidden' name='action' value='delete'>";
    echo "  <input type='button' value='Delete' onclick='sendDeleteRequest()'>";
    echo "</form>";

}


echo "</body></html>";
?>

<script>
    function sendDeleteRequest() {
        // Use JavaScript to send a DELETE request when the "Delete" button is clicked
        fetch('/cgi-bin/cgi.php', {
            method: 'DELETE',
            headers: {
                'Content-Type': 'application/json',
                // Add any other headers as needed
            },
            body: JSON.stringify({
                // Add any data you want to send with the DELETE request
            }),
        })
        .then(response => {
            if (!response.ok) {
                throw new Error('Network response was not ok');
            }
            // Handle success, if needed
        })
        .catch(error => console.error('Error:', error));
    }
</script>
