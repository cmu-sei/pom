# How to release POM to Github

## Copyright

<legal>  
Pointer Ownership Model (POM) Source Code Release  
  
Copyright 2025 Carnegie Mellon University.  
  
NO WARRANTY. THIS CARNEGIE MELLON UNIVERSITY AND SOFTWARE ENGINEERING  
INSTITUTE MATERIAL IS FURNISHED ON AN "AS-IS" BASIS. CARNEGIE MELLON  
UNIVERSITY MAKES NO WARRANTIES OF ANY KIND, EITHER EXPRESSED OR  
IMPLIED, AS TO ANY MATTER INCLUDING, BUT NOT LIMITED TO, WARRANTY OF  
FITNESS FOR PURPOSE OR MERCHANTABILITY, EXCLUSIVITY, OR RESULTS  
OBTAINED FROM USE OF THE MATERIAL. CARNEGIE MELLON UNIVERSITY DOES NOT  
MAKE ANY WARRANTY OF ANY KIND WITH RESPECT TO FREEDOM FROM PATENT,  
TRADEMARK, OR COPYRIGHT INFRINGEMENT.  
  
Licensed under a MIT (SEI)-style license, please see license.txt or  
contact permission@sei.cmu.edu for full terms.  
  
[DISTRIBUTION STATEMENT A] This material has been approved for public  
release and unlimited distribution.  Please see Copyright notice for  
non-US Government use and distribution.  
  
DM25-1262  
</legal>  

## Release Process

The process for releasing POM to Github is manual and should be done every time an update to Github is required.  Currently POM is located on [SEI's Bitbucket repo](https://bitbucket.cc.cert.org/bitbucket/projects/POM/repos/pointer-ownership-model/browse), and this process puts the latest version onto the [POM Github repo](https://github.com/cmu-sei/pom). The steps are as follows.

1. Clone a pristine version of POM.  This might be the main branch or a branch specifically for the release.

2. Remove the following directories and files, these do not go into the release:
  * design-extracts
  * paper
  * release_process.md (this file)

3. Recreate the `manifest.txt` file, this command works, in the top-level directory:

``` bash
find . -type f -print | sort -u > manifest.txt
```

But then you'll need to remove the `.git` files from the manifest.

4. Run a Docker container that has `/datasets/update-markings` contain the update-markings distribution from its [Bitbucket repo](https://bitbucket.cc.cert.org/bitbucket/projects/SCS/repos/update-markings/browse).  This is one such command:

``` bash
docker run -it --rm  -v ${PWD}:/host -w /host -v ~/Public/Work/checkouts/update-markings:/update-markings  docker.cc.cert.org/pom/pointer-ownership-model:latest  bash 
```

5. Identify unknown files & files missing legal tags using this command:

``` bash
python3 /update-markings/src/update-markings.py -d --no-cui ${PWD}/update-markings.yml
```

Add missing legal tags and any unknown files until this script issues no warnings.

6. Then run the update-markings test, to make sure the manifest file is correct:

``` bash
python3 -m pytest -v /update-markings/src/test-update-markings.py --config ${PWD}/update-markings.yml
```

7. Commit any changes back to SEI Bitbucket

8. Run the update-markings script without the '-d' option. This updates every file with the current copyright.

``` bash
python3 /update-markings/src/update-markings.py --no-cui ${PWD}/update-markings.yml
```

9. Run some tests to confirm that this change broke nothing.

10. Then remove the `.git` directory.

11. Finally, commit updates to the Github repo.
