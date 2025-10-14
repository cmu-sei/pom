# Which LLM should POM use?

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

## Constraints

The choice of LLM that POM should use is subject to the following constraints:

### Quality

According to the paper [Evaluating the code quality of ai-assisted code generation tools](https://arxiv.org/abs/2304.10778) (see pg 12), ChatGPT outperformed CoPilot and Amazon CodeWhisperer at several code correctness tasks.

One might argue that a lower-quality LLM might be more interesting to use, especially because its results will be fed to the POM Verifier, which should flag hallucinations as well as errors. (Technically, it does not distinguish between LLM hallucinations and other kinds of errors.)

### Training Data

Each LLM is trained using training data.   Some of this training is considered proprietary and secret. A good intro, including training data for prominent LLMs is available at [Knowledge Cutoff Dates of all LLMs explained](https://otterly.ai/blog/knowledge-cutoff/).

Since POM is a research project, we should make sure that the LLM has not been exposed to our evaluation test suite, which is best done by verifying that the data of publication of (at least part of) the test suite is later than the LLM's training data.  This suggests that older LLMs would be better to use than newer ones.
