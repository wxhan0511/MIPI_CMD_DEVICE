Set-Location 'D:\Project\MIPI_Cmd_Device'
foreach($f in @('i2c_task.c','gtb_task.c','com_define.h','cdc_task.c','gtb_task.h','power_task.c','pwm_ctrl.c','task_com.c','task_com.h','task_sample.c','task_sample.h','com_handle.c','com_handle.h')){
  git cat-file -p "HEAD:Task/$f" > "$env:TEMP\blob.tmp"
  $b = [System.IO.File]::ReadAllBytes("$env:TEMP\blob.tmp")
  $crlf=0; $lf=0
  for($i=0; $i -lt $b.Length; $i++){
    if($b[$i] -eq 10){
      if($i -gt 0 -and $b[$i-1] -eq 13){ $crlf++ } else { $lf++ }
    }
  }
  "$f : HEAD-blob CRLF=$crlf loneLF=$lf"
}
Remove-Item "$env:TEMP\blob.tmp"
