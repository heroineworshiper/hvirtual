;;  ----------------------------------------------------------------  ;;
;;                 Nagoya Institute of Technology and                 ;;
;;                     Carnegie Mellon University                     ;;
;;                         Copyright (c) 2002                         ;;
;;                        All Rights Reserved.                        ;;
;;                                                                    ;;
;;  Permission is hereby granted, free of charge, to use and          ;;
;;  distribute this software and its documentation without            ;;
;;  restriction, including without limitation the rights to use,      ;;
;;  copy, modify, merge, publish, distribute, sublicense, and/or      ;;
;;  sell copies of this work, and to permit persons to whom this      ;;
;;  work is furnished to do so, subject to the following conditions:  ;;
;;                                                                    ;;
;;    1. The source code must retain the above copyright notice,      ;;
;;       this list of conditions and the following disclaimer.        ;;
;;                                                                    ;;
;;    2. Any modifications to the source code must be clearly         ;;
;;       marked as such.                                              ;;
;;                                                                    ;;
;;    3. Original authors' names are not deleted.                     ;;
;;                                                                    ;;
;;    4. The authors' names are not used to endorse or promote        ;;
;;       products derived from this software without specific prior   ;;
;;       written permission.                                          ;;
;;                                                                    ;;
;;  NAGOYA INSTITUTE OF TECHNOLOGY, CARNEGIE MELLON UNIVERSITY AND    ;;
;;  THE CONTRIBUTORS TO THIS WORK DISCLAIM ALL WARRANTIES WITH        ;;
;;  REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF      ;;
;;  MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL NAGOYA INSTITUTE   ;;
;;  OF TECHNOLOGY, CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS    ;;
;;  BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR   ;;
;;  ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        ;;
;;  PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    ;;
;;  TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR  ;;
;;  PERFORMANCE OF THIS SOFTWARE.                                     ;;
;;                                                                    ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;     A voice based on "HTS" HMM-Based Speech Synthesis System.      ;;
;;          Author :  Alan W Black                                    ;;
;;          Date   :  August 2002                                     ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; Try to find the directory where the voice is, this may be from
;;; .../festival/lib/voices/ or from the current directory
(if (assoc 'nitech_us_clb_arctic_hts voice-locations)
    (defvar nitech_us_clb_arctic_hts::hts_dir 
      (cdr (assoc 'nitech_us_clb_arctic_hts voice-locations)))
    (defvar nitech_us_clb_arctic_hts::hts_dir (string-append (pwd) "/")))

(defvar nitech_us_clb_arctic::clunits_dir nitech_us_clb_arctic_hts::hts_dir)
(defvar nitech_us_clb_arctic::clunits_loaded nil)

;;; Did we succeed in finding it
(if (not (probe_file (path-append nitech_us_clb_arctic_hts::hts_dir "festvox/")))
    (begin
     (format stderr "nitech_us_clb_arctic_hts::hts: Can't find voice scm files they are not in\n")
     (format stderr "   %s\n" (path-append  nitech_us_clb_arctic_hts::hts_dir "festvox/"))
     (format stderr "   Either the voice isn't linked in Festival library\n")
     (format stderr "   or you are starting festival in the wrong directory\n")
     (error)))

;;;  Add the directory contains general voice stuff to load-path
(set! load-path (cons (path-append nitech_us_clb_arctic_hts::hts_dir "festvox/") 
		      load-path))

(set! hts_data_dir (path-append nitech_us_clb_arctic_hts::hts_dir "hts/"))

(set! hts_feats_list
      (load (path-append hts_data_dir "feat.list") t))

(require 'hts)
(require_module 'hts_engine)

;;; Voice specific parameter are defined in each of the following
;;; files
(require 'nitech_us_clb_arctic_phoneset)
(require 'nitech_us_clb_arctic_tokenizer)
(require 'nitech_us_clb_arctic_tagger)
(require 'nitech_us_clb_arctic_lexicon)
(require 'nitech_us_clb_arctic_phrasing)
(require 'nitech_us_clb_arctic_intonation)
(require 'nitech_us_clb_arctic_duration)
(require 'nitech_us_clb_arctic_f0model)
(require 'nitech_us_clb_arctic_other)
;; ... and others as required


(define (nitech_us_clb_arctic_hts::voice_reset)
  "(nitech_us_clb_arctic_hts::voice_reset)
Reset global variables back to previous voice."
  (nitech_us_clb_arctic::reset_phoneset)
  (nitech_us_clb_arctic::reset_tokenizer)
  (nitech_us_clb_arctic::reset_tagger)
  (nitech_us_clb_arctic::reset_lexicon)
  (nitech_us_clb_arctic::reset_phrasing)
  (nitech_us_clb_arctic::reset_intonation)
  (nitech_us_clb_arctic::reset_duration)
  (nitech_us_clb_arctic::reset_f0model)
  (nitech_us_clb_arctic::reset_other)

  t
)

(set! nitech_us_clb_arctic_hts::hts_feats_list
      (load (path-append hts_data_dir "feat.list") t))

(set! nitech_us_clb_arctic_hts::hts_engine_params
      (list
       (list "-dm1" (path-append hts_data_dir "mcep_dyn.win"))
       (list "-dm2" (path-append hts_data_dir "mcep_acc.win"))
       (list "-df1" (path-append hts_data_dir "lf0_dyn.win"))
       (list "-df2" (path-append hts_data_dir "lf0_acc.win"))
       (list "-td" (path-append hts_data_dir "trees-dur.inf"))
       (list "-tm" (path-append hts_data_dir "trees-mcep.inf"))
       (list "-tf" (path-append hts_data_dir "trees-lf0.inf"))
       (list "-md" (path-append hts_data_dir "duration.pdf"))
       (list "-mm" (path-append hts_data_dir "p_mcep.pdf"))
       (list "-mf" (path-append hts_data_dir "lf0.pdf"))
       '("-a"   0.420000)                 
       '("-r"   0.000000)                 
       '("-fs"  1.000000)             
       '("-fm"  0.000000)            
       '("-u"   0.500000)                 
       '("-l"   0.000000) 
       ))

;; This function is called to setup a voice.  It will typically
;; simply call functions that are defined in other files in this directory
;; Sometime these simply set up standard Festival modules othertimes
;; these will be specific to this voice.
;; Feel free to add to this list if your language requires it

(define (voice_nitech_us_clb_arctic_hts)
  "(voice_nitech_us_clb_arctic_hts)
Define voice for limited domain: us."
  ;; *always* required
  (voice_reset)

  ;; Select appropriate phone set
  (nitech_us_clb_arctic::select_phoneset)

  ;; Select appropriate tokenization
  (nitech_us_clb_arctic::select_tokenizer)

  ;; For part of speech tagging
  (nitech_us_clb_arctic::select_tagger)

  (nitech_us_clb_arctic::select_lexicon)
  ;; For hts selection you probably don't want vowel reduction
  ;; the unit selection will do that
  (if (string-equal "americanenglish" (Param.get 'Language))
      (set! postlex_vowel_reduce_cart_tree nil))

  (nitech_us_clb_arctic::select_phrasing)

  (nitech_us_clb_arctic::select_intonation)

  (nitech_us_clb_arctic::select_duration)

  (nitech_us_clb_arctic::select_f0model)

  ;; Waveform synthesis model: hts
  (set! hts_engine_params nitech_us_clb_arctic_hts::hts_engine_params)
  (set! hts_feats_list nitech_us_clb_arctic_hts::hts_feats_list)
  (Parameter.set 'Synth_Method 'HTS)

  ;; This is where you can modify power (and sampling rate) if desired
  (set! after_synth_hooks nil)
;  (set! after_synth_hooks
;      (list
;        (lambda (utt)
;          (utt.wave.rescale utt 2.1))))

  (set! current_voice_reset nitech_us_clb_arctic_hts::voice_reset)

  (set! current-voice 'nitech_us_clb_arctic_hts)
)

(proclaim_voice
 'nitech_us_clb_arctic_hts
 '((language english)
   (gender female)
   (dialect american)
   (description
    "US English female speaker CLB.")))

(provide 'nitech_us_clb_arctic_hts)

