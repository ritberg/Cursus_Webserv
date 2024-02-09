<?php
$result = '';

// Check if the form has been submitted
if ($_SERVER['REQUEST_METHOD'] == 'POST') {
    // Retrieve the last line from the POST data
    $postData = file_get_contents("php://input");
    $lines = explode("\n", $postData);
    $lastLine = end($lines);

    // Parse the last line to get the values
    parse_str($lastLine, $formData);

    // Perform the calculation based on the values
    $num1 = isset($formData['num1']) ? (float)$formData['num1'] : 0;
    $num2 = isset($formData['num2']) ? (float)$formData['num2'] : 0;
    $operator = isset($formData['operator']) ? $formData['operator'] : '';

    // Perform the calculation based on the operator
    switch ($operator) {
        case 'add':
            $result = $num1 + $num2;
            break;
        case 'subtract':
            $result = $num1 - $num2;
            break;
        case 'multiply':
            $result = $num1 * $num2;
            break;
        case 'divide':
            // Handle division by zero
            $result = ($num2 != 0) ? $num1 / $num2 : 'Cannot divide by zero';
            break;
        default:
            $result = 'Invalid operator';
    }
}
?>

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Simple Calculator</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            margin-top: 50px;
        }

        input, select, button {
            margin: 10px;
            font-size: 16px;
        }
    </style>
</head>
<body>

<form method="post" action="">
    <input type="number" name="num1" placeholder="Enter number 1" required>
    <select name="operator" required>
        <option value="add">+</option>
        <option value="subtract">-</option>
        <option value="multiply">*</option>
        <option value="divide">/</option>
    </select>
    <input type="number" name="num2" placeholder="Enter number 2" required>
    <button type="submit" name="submit">Calculate</button>
</form>

<?php if ($result !== ''): ?>
    <h2>Result: <?php echo $result; ?></h2>
<?php endif; ?>

</body>
</html>
