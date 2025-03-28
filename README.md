# cpu_prefetch_control

## Compilation
make

## View Help Information
./prefetch_control -h

## Example Usage:
### Disable prefetch for core 0
sudo ./prefetch_control -c 0 -d

### Enable prefetch for core 1
sudo ./prefetch_control -c 1 -e

### Check prefetch status for core 2
sudo ./prefetch_control -c 2 -s

### Disable prefetch for all cores
sudo ./prefetch_control -a -d