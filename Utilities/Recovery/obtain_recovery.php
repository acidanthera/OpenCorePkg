<?php

function run_query($headerQuery, $httpQuery, &$headers, &$output, $postvars='', $verbose=false) {
	$ch = curl_init();

	$headers = [];

	curl_setopt($ch, CURLOPT_URL, $httpQuery);
	curl_setopt($ch, CURLOPT_FOLLOWLOCATION, true);
	curl_setopt($ch, CURLOPT_TIMEOUT, 99999*60);

	if (strlen($postvars) > 0) {
		curl_setopt($ch, CURLOPT_POST, true);
		curl_setopt($ch, CURLOPT_POSTFIELDS, $postvars);
	}

	if ($verbose) {
		$f = tmpfile();
		curl_setopt($ch, CURLOPT_VERBOSE, true);
		curl_setopt($ch, CURLOPT_STDERR, $f);
	}

	$tofile = is_resource($output) && (get_resource_type($output) === 'stream' || get_resource_type($output) === 'file');

	curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);

	if ($tofile)
		curl_setopt($ch, CURLOPT_FILE, $output);

	curl_setopt($ch, CURLOPT_HEADERFUNCTION, function($curl, $header) use (&$headers) {
		$len = strlen($header);
		$header = explode(':', $header, 2);
		if (count($header) < 2) // ignore invalid headers
			return $len;

		$name = strtolower(trim($header[0]));
		if (!array_key_exists($name, $headers))
			$headers[$name] = [trim($header[1])];
		else
			$headers[$name][] = trim($header[1]);

		return $len;
	});


	curl_setopt($ch, CURLOPT_HTTPHEADER, $headerQuery);

	if ($tofile)
		curl_exec($ch);
	else
		$output = curl_exec($ch);

	curl_close($ch);

	if ($verbose) {
		fseek($f, 0);
		echo fread($f, 32*1024);
		fclose($f);
	}
}

function dump_query($name, $headers, $output) {
	print 'Performed: ' . $name . PHP_EOL;

	foreach ($headers as $name => $value) {
		print $name . ': ' . $value[0] . PHP_EOL;
	}

	print PHP_EOL . 'Output:' . PHP_EOL;
	print $output . PHP_EOL;
}

function setup_session() {
	$headersReq = [
		'Host: osrecovery.apple.com',
		'Connection: close',
		'User-Agent: InternetRecovery/1.0'
	];

	$output = '';
	$headers = [];

	run_query($headersReq, 'http://osrecovery.apple.com/', $headers, $output);
	dump_query('setup_session', $headers, $output);

	$cookie = '';

	foreach ($headers as $name => $value) {
		if ($name == 'set-cookie') {
			$cookie = explode(';', $value[0])[0];
			break;
		}
	}

	return $cookie;
}

function obtain_images($session, $mlb, $board) {
	$headersReq = [
		'Host: osrecovery.apple.com',
		'Connection: close',
		'User-Agent: InternetRecovery/1.0',
		'Cookie: ' . $session,
		'Content-Type: text/plain',
		'Expect:'
	];

	$output = '';
	$headers = [];

	$postvars =
		'cid=F4FDBCCF36190DD4' . PHP_EOL .
		// MLB, board serial number
		'sn=' . $mlb . PHP_EOL .
		// board-id
		'bid=' . $board . PHP_EOL .
		'k=6E7D753C11E1F9652B99D3DB8C80A49E82143EA027CBA516E3E18B3A4FFDCD58' . PHP_EOL .
		'fg=80F6E802A09B8B553202EE0D37AE64662ACFAF30B111E1984C01F64551BB7EFE'
	;

	$images = [ 'image' => [], 'chunklist' => [] ];

	// /InstallationPayload/Diagnostics
	run_query($headersReq, 'http://osrecovery.apple.com/InstallationPayload/RecoveryImage', $headers, $output, $postvars);
	dump_query('obtain_images', $headers, $output);

	$fields = explode("\n", $output);

	foreach ($fields as $field) {
		$pair = explode(': ', $field);
		if (count($pair) != 2)
			continue;

		$name = $pair[0];
		$value = $pair[1];

		if ($name == 'AU')
			$images['image']['link'] = $value;
		else if ($name == 'AH')
			$images['image']['hash'] = $value;
		else if ($name == 'AT')
			$images['image']['cookie'] = $value;
		else if ($name == 'CU')
			$images['chunklist']['link'] = $value;
		else if ($name == 'CH')
			$images['chunklist']['hash'] = $value;
		else if ($name == 'CT')
			$images['chunklist']['cookie'] = $value;
	}

	return $images;
}

function download_images($images) {
	foreach ($images as $imagename => $imagefields) {
		$headersReq = [
			'Host: ' . parse_url($imagefields['link'], PHP_URL_HOST),
			'Connection: close',
			'User-Agent: InternetRecovery/1.0',
			'Cookie: AssetToken=' . $imagefields['cookie']
		];

		if ($imagename == 'image')
			$filename = 'RecoveryImage.dmg';
		else
			$filename = 'RecoveryImage.chunklist';

		$headers = [];
		$output = fopen($filename, 'w+');

		run_query($headersReq, $imagefields['link'], $headers, $output);

		fclose($output);
	}
}

if ($argc != 3) {
	print 'Usage: php obtain_recovery.php MLB board-id' . PHP_EOL;
	exit(1);
}

$sess = setup_session();
if ($sess == '') {
	print 'Failed to obtain session!' . PHP_EOL;
	exit(1);
}

$images = obtain_images($sess, $argv[1], $argv[2]);

if (count($images['image']) == 0 || count($images['chunklist']) == 0) {
	print 'Failed to obtain images!' . PHP_EOL;
	exit(1);
}

download_images($images);
