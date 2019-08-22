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

function obtain_images($session, $board, $mlb, $type = 'default', $diag = false) {
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

	// Recovery latest:
	// cid=3076CE439155BA14
	// sn=...
	// bid=Mac-E43C1C25D4880AD6
	// k=4BE523BB136EB12B1758C70DB43BDD485EBCB6A457854245F9E9FF0587FB790C
	// os=latest
	// fg=B2E6AA07DB9088BE5BDB38DB2EA824FDDFB6C3AC5272203B32D89F9D8E3528DC
	//
	// Recovery default:
	// cid=4A35CB95FF396EE7
	// sn=...
	// bid=Mac-E43C1C25D4880AD6
	// k=0A385E6FFC3DDD990A8A1F4EC8B98C92CA5E19C9FF1DD26508C54936D8523121
	// os=default
	// fg=B2E6AA07DB9088BE5BDB38DB2EA824FDDFB6C3AC5272203B32D89F9D8E3528DC
	//
	// Diagnostics:
	// cid=050C59B51497CEC8
	// sn=...
	// bid=Mac-E43C1C25D4880AD6
	// k=37D42A8282FE04A12A7D946304F403E56A2155B9622B385F3EB959A2FBAB8C93
	// fg=B2E6AA07DB9088BE5BDB38DB2EA824FDDFB6C3AC5272203B32D89F9D8E3528DC
	//
	// Server logic for recovery:
	// if not valid(bid):
	//   return error()
	// ppp = get_ppp(sn)
	// if not valid(ppp):
	//   return latest_recovery(bid = bid)             # Returns newest for bid.
	// if valid(sn):
	//   if os == 'default':
	//     return default_recovery(sn = sn, ppp = ppp) # Returns oldest for sn.
	//   else:
	//     return latest_recovery(sn = sn, ppp = ppp)  # Returns newest for sn.
	// return default_recovery(ppp = ppp)              # Returns oldest.

	$postvars =
		// Context ID, should be random
		'cid=F4FDBCCF36190DD4' . PHP_EOL .
		// MLB, board serial number
		'sn=' . $mlb . PHP_EOL .
		// board-id
		'bid=' . $board . PHP_EOL .
		// FIXME: Should be random
		'k=6E7D753C11E1F9652B99D3DB8C80A49E82143EA027CBA516E3E18B3A4FFDCD58' . PHP_EOL .
		($diag ? '' : 'os=' . $type . PHP_EOL) .
		'fg=80F6E802A09B8B553202EE0D37AE64662ACFAF30B111E1984C01F64551BB7EFE'
	;

	if ($diag)
		$url = 'http://osrecovery.apple.com/InstallationPayload/Diagnostics';
	else
		$url = 'http://osrecovery.apple.com/InstallationPayload/RecoveryImage';

	$images = [ 'image' => [], 'chunklist' => [] ];

	run_query($headersReq, $url, $headers, $output, $postvars);
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

function download_images($images, $diag = false) {
	foreach ($images as $imagename => $imagefields) {
		$headersReq = [
			'Host: ' . parse_url($imagefields['link'], PHP_URL_HOST),
			'Connection: close',
			'User-Agent: InternetRecovery/1.0',
			'Cookie: AssetToken=' . $imagefields['cookie']
		];

		if ($diag)
			$type = 'Diagnostics';
		else
			$type = 'Recovery';

		if ($imagename == 'image')
			$filename = $type . 'Image.dmg';
		else
			$filename = $type . 'Image.chunklist';

		$headers = [];
		$output = fopen($filename, 'w+');

		print $imagefields['link'] . ' ' . $filename . PHP_EOL;

		run_query($headersReq, $imagefields['link'], $headers, $output);

		fclose($output);
	}
}

if ($argc < 2) {
	print 'Usage: php obtain_recovery.php board-id [MLB] [--diag] [--latest]' . PHP_EOL;
	exit(1);
}

$board = $argv[1];
$mlb   = '00000000000000000';
$type  = 'default';

if ($argc > 2) {
	if ($argv[2] == '--diag') {
		$diag = true;
	} else if ($argv[2] == '--latest') {
		$type = 'latest';
	} else {
		$mlb  = $argv[2];
		$diag = $argc > 3 && $argv[3] == '--diag'
		  || $argc > 4 && $argv[4] == '--diag';
		if ($argc > 3 && $argv[3] == '--latest'
		  || $argc > 4 && $argv[4] == '--latest')
		  $type = 'latest';
	}
}

print 'Downloading '.$type.' '.($diag ? 'diagnostics' : 'recovery').' for '.$board.' with '.$mlb.PHP_EOL;

$sess = setup_session();
if ($sess == '') {
	print 'Failed to obtain session!' . PHP_EOL;
	exit(1);
}

$images = obtain_images($sess, $board, $mlb, $type, $diag);

if (count($images['image']) == 0 || count($images['chunklist']) == 0) {
	print 'Failed to obtain images!' . PHP_EOL;
	exit(1);
}

download_images($images, $diag);
