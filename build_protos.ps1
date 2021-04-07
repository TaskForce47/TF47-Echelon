$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User");
$currentDir = Get-Location;
$files = [System.IO.Directory]::GetFiles([System.IO.Path]::Combine($currentDir, "protos"));

cd protos;

foreach ($file in $files) {

	$fileInfo = [System.IO.FileInfo]::new($file);
	if ($fileInfo.Extension -ne ".proto") {
		Write-Output "Cleaning up" $file;
		[System.IO.File]::Delete($file);	
	}
}

foreach ($file in $files) {

	$fileInfo = [System.IO.FileInfo]::new($file);
	if ($fileInfo.Extension -eq ".proto") {
		Write-Output "Generating for file:" $file;
		protoc.exe -I="." --cpp_out="." $fileInfo.Name
		protoc -I="." --grpc_out="." --plugin=protoc-gen-grpc="S:\Development\vcpkg\installed\x64-windows\tools\grpc\grpc_cpp_plugin.exe" $fileInfo.Name
	}
}

cd ..
Write-Output "Done!";