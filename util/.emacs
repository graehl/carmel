(defun kill-whole-line ()
  "delete from start of current line instead of cursor as per normal kill-line"
    (interactive)
    (let ((c (current-column)))
    (beginning-of-line)
    (kill-line)
    (move-to-column c))
  )
(global-set-key (kbd "M-k") 'kill-whole-line)
(global-set-key (kbd "C-M-k") 'kill-sentence)
(global-set-key (kbd "C-k") 'kill-line)
;(add-hook 'c-mode-common-hook (lambda () (setq c-hungry-delete-key t)))


(defvar use-longlines nil)
(push "~/elisp/scala" load-path)
(require 'scala-mode-auto)
(defun me-turn-off-indent-tabs-mode () (setq indent-tabs-mode nil))
(defun my-scala-eval-line ()
  "eval current line"
  (interactive)
  (scala-eval-region (line-beginning-position) (line-end-position))
  )
(defun my-scala-eval ()
  "Send the current 'definition' to the Scala interpreter.
   Cursor is on a line that doesn't start w/ ws or }: eval current line
   or else: find prev and next such line that starts w/ non ws { }.  That's the start/end of a defn.
"
  (interactive)
  (save-excursion
    ;; find the first non-empty line
    (beginning-of-line)
    (while (and (not (= (point) (point-min)))
                (looking-at "\\s-*$"))
      (next-line -1))
    (while and (not (= (point) (point-max))) (looking-at "\\s") (next-line 1))
    (end-of-line)
    (let ((end (point)))
      ;; now we need to find the start
      (beginning-of-line)
      (while (and (not (= (point) (point-min)))
                  (looking-at (mapconcat '(lambda (x) x)
                                         '("^$"       ; empty lines
                                           "^\\s-+"   ; empty lines or lines that start with whitespace
                                           "^\\s-*}") ; lines that start with a '}'
                                         "\\|")))
        (next-line -1)
        (beginning-of-line))
      (message "region %s %s" (point) end)
      (scala-eval-region (point) end))))
(add-hook 'scala-mode-hook 'me-turn-off-indent-tabs-mode)
(add-hook 'scala-mode-hook (lambda () (local-set-key (kbd "M-;") 'my-scala-eval-line) (local-set-key (kbd "M-'") 'scala-eval-region)))
(add-to-list 'auto-mode-alist '("\\.scala$" . scala-mode))

(defvar emacs23 (string-match "Emacs 23" (emacs-version)))
(require 'cl)
(require 'cl-19)
(defun prev-line (n) (forward-line (- n)))
(defun bufend() (goto-char (point-max)))
(defun bufstart() (goto-char (point-min)))
                                        ; whole buffer
(defun subst-re (m r &optional start end)
  (setq start (or start (point-min)))
  (setq end (or end (point-max)))
  (if emacs23
      (save-excursion
        (goto-char start)
        (while (and (< (point) end) (re-search-forward m end t)) (replace-match r)))
    (replace-regexp m r nil start end)))

(setq win32-tempdir "c:/windows/temp")
(defvar on-xemacs (string-match "XEmacs\\|Lucid" (emacs-version)))
(defvar on-emacs (not on-xemacs))
(setq on-win32
      (or (eq system-type 'windows-nt)
          (eq system-type 'cygwin32)
          (eq system-type 'cygwin)))
;(defvar on-win32 (string-match "mingw\\|win32\\|nt5" (emacs-version)))
(defvar on-win32-xemacs (and on-win32 on-xemacs))
(defvar on-win32-emacs (and on-win32 on-emacs))
(defvar on-linux (string-match "linux" (emacs-version)))
(defvar on-unix (not on-win32))
(defvar not-on-a-tty (or (not (fboundp 'device-type))
                         (equal (device-type) 'x)
                         (equal (device-type) 'mswindows)))
(defvar recent-emacs nil)
(defvar emacs-22 (string-match "Emacs 22." (emacs-version)))
(defvar recent-xemacs on-xemacs)
(defvar cygwin-xemacs on-xemacs)
(defvar xemacs-21-5 (string-match "XEmacs 21.5" (emacs-version)))
(setq on-emacs (not (string-match "^\\(XEmacs\\).*" (emacs-version))))

(when on-win32-emacs (set-message-beep 'silent)) ; 'silent 'ok
(setq visible-bell t)
(if emacs-22 (global-set-key (kbd "<f9>") 'lgrep)
  (global-set-key (kbd "<f9>") 'grep))

(setq font-lock-maximum-decoration t)

(setq lazy-lock-defer-after-change t)
(if (fboundp 'global-font-lock-mode)
    (global-font-lock-mode 1)        ; GNU Emacs
  (setq font-lock-auto-fontify t))   ; XEmacs

;(load "~/elisp/xemacs.bak/func-menu.el")
      ;; The docs say that this is deprecated, but so far it's the
      ;; only way to make things work right.  Which things?  I don't
      ;; remember anymore.  It's certainly breaking things under
      ;; XEmacs.  Turned off everywhere until I discover a problem
;      (and nil on-emacs (setq directory-sep-char ?/))
;(setq directory-sep-char ?/)

;; font-lock-mode is what implements syntax coloring and hilighting.
(require 'font-lock)            ; require forces the module to load right away.
                                        ; .emacs file font-lock stuff

(defun my-recenter (&optional arg)
  "Centre point in window and run font-lock-fontify-block"
  (interactive "P")
                                        ;  (my-fontlock)
  (recenter arg)
  (and on-emacs (font-lock-fontify-block (window-height)))
  (and on-xemacs (font-lock-default-fontify-buffer))
  (and use-longlines (longlines-mode 1))
  )

(add-hook 'font-lock-mode-hook
          '(lambda ()
                                        ;                (my-fontlock)
             (substitute-key-definition
              'recenter 'my-recenter (current-global-map))))


(defun pbug ()
  "Check parenthesis bugs or similar horrors.

Even with Emacs advanced programming facilities, checking mismatching
parenthesis or missing quote (so called \"pbug\") is no less annoying than
pointer chasing in C.

This function divides the buffer into regions and tries evaluating them one
by one.  It stops at the first region where it fails to evaluate because of
pbug or any other errors. It sets point and mark (and highlights if
`transient-mark-mode' is on) on the failing region and center its first
line.  \"^def\" is used to define regions.  You may also `eval-region'
right after pbug is done to let lisp parse pinpoint the bug.

No more \"End of file during parsing\" horrors!"
  (interactive)
  (let ((point (point))
        (region-regex "^(def..")
        defs beg end)
    (goto-char (point-min))
    (setq defs (loop while (search-forward-regexp region-regex nil t)
                     collect (point-at-bol)))
    ;; so it evals last definition
    (nconc defs (list (point-max)))
    (setq beg (point-min))
    (while defs
      (goto-char beg)
      (setq end (pop defs))
      ;; to be cool, uncomment these to see pbug doing step by step
      ;; (message "checking pbug from %s to %s..." beg end)
      ;; (sit-for 1)
      (when (eq (condition-case nil
                    (eval-region beg (1- end))
                  (error 'pbug-error))
                'pbug-error)
        (push-mark end 'nomsg 'activate)
        (goto-char beg)
        (recenter)
        (error "a pbug found from %s to %s" beg end))
      (setq beg end))
    (goto-char point)
    (message "no pbug found")))
(define-key emacs-lisp-mode-map [(meta p)] 'pbug)

(require 'compile)


(defun my-backward-kill-word ()
  "Kill words backward my way."
  (interactive)
  (if (bolp)
      (backward-delete-char 1)
    (if (string-match "^\\s-+$" (buffer-substring (point-at-bol) (point)))
        (kill-region (point-at-bol) (point))
      (backward-kill-word 1))))

;;; Stefan Monnier <foo at acm.org>. It is the opposite of
;;; fill-paragraph. Takes a multi-line paragraph and makes
;;; it into a single line of text.
(defun unfill-paragraph ()
  (interactive)
  (let ((fill-column (point-max)))
    (fill-paragraph nil)))

;(setq compile-command "bash -c '. ~/.bashrc;ecage variant=debug utility=mini_decoder boostsbmt'")
;(setq compile-command "bash -c '. ~/.bashrc;ecage  utility=mini_decoder build_sbmt_variant --mini-ngram-order=3'")
;(setq compile-command "bash -c '. ~/.bashrc;ecage \"cd ~/t/graehl/tt &&  make install BOOST_SUFFIX=gcc42\"'")
;(setq compile-command "bash --login -c make")
;     (setq compile-command "bash -c '. ~/.bashrc;eerdos variant=debug boostsbmt --mini-ngram-order=3'")
;    (setq compile-command "bash -c '. ~/.bashrc;egrieg \"cd ~/t/graehl/carmel &&  make install BOOST_SUFFIX=gcc42\"'")
;    (setq compile-command "bash -c '. ~/.bashrc;ecage \"cd ~/t/graehl/carmel &&  make install BOOST_SUFFIX=gcc42\"'")
;     (setq compile-command "ssh nlg0 'cd dev/sbmt_decoder;make -j 2 && cd ~/dev/utilities && make -j 2'")

(setq debug-on-error 't)

(unless (fboundp 'line-beginning-position)
    (defun line-beginning-position (&optional n)
          "Return the `point' of the beginning of the current line."
	        (save-excursion
		        (beginning-of-line n)
			        (point))))

;; If `line-end-position' isn't available provide one.
(unless (fboundp 'line-end-position)
  (defun line-end-position (&optional n)
    "Return the `point' of the end of the current line."
    (save-excursion
      (end-of-line n)
      (point))))

;; The regexp "\\s-+$" is too general, since form feeds (\n), carriage
;; returns (\r), and form feeds/page breaks (C-l) count as whitespace in
;; some syntaxes even though they serve a functional purpose in the file.
(defconst whitespace-regexp "[ \t]+$"
  "Regular expression which matches trailing whitespace.")

;; Match two or more trailing newlines at the end of the buffer; all but
;; the first newline will be deleted.
(defconst whitespace-eob-newline-regexp "\n\n+\\'"
  "Regular expression which matches newlines at the end of the buffer.")

(add-hook 'before-save-hook 'delete-trailing-whitespace)
(require 'time-stamp)
(add-hook 'before-save-hook 'time-stamp)


;(require 'w32-symlinks)
;(customize-option 'w32-symlinks-handle-shortcuts)
(if nil
    ;on-win32
    (progn
                        (setenv "PATH" (concat "c:/cygwin/bin;" (getenv "PATH")))
                        (setq exec-path (cons "c:/cygwin/bin/" exec-path))
;                        (require 'cygwin-mount)
;                        (cygwin-mount-activate)
                        ))

;(push "~/elisp/cc-mode" load-path)
;(push "~/elisp/cc-mode" load-path)
(and (not xemacs-21-5) on-xemacs (progn (push "~/elisp/xemacs" load-path) (push "~/elisp/cc-mode/xemacs" load-path)))
(require 'cc-mode)
(custom-set-variables
  ;; custom-set-variables was added by Custom.
  ;; If you edit it by hand, you could mess it up, so be careful.
  ;; Your init file should contain only one such instance.
  ;; If there is more than one, they won't work right.
 '(c-block-comment-prefix "")
 '(compilation-ask-about-save nil)
 '(compilation-auto-jump-to-first-error nil)
 '(compilation-disable-input t)
 '(compilation-error-regexp-alist (quote (absoft ada aix ant bash borland caml comma edg-1 edg-2 epc ftnchek iar ibm irix java jikes-file jikes-line gnu gcc-include lcc makepp mips-1 mips-2 msft omake oracle perl php rxp sparc-pascal-file sparc-pascal-line sparc-pascal-example sun sun-ada watcom 4bsd gcov-file gcov-header gcov-nomark gcov-called-line gcov-never-called perl--Pod::Checker perl--Test perl--Test2 perl--Test::Harness weblint)))
 '(compilation-scroll-output (quote first-error))
 '(compilation-search-path (quote (nil "~/t/graehl/shared" "~/t/sbmt_decoder/src" "~/t/sbmt_decoder/include/sbmt" "~/t/sbmt_decoder/include/sbmt/edge" "~/t/sbmt_decoder/include/sbmt/ngram" "~/t/sbmt_decoder/include/sbmt/edge/impl" "~/isd/boost/boost/iterator" "~/t/sbmt_decoder/include/sbmt/feature" "~/t/utilities" "~/t/graehl/carmel/src")))
 '(compilation-skip-threshold 2)
 '(ensime-default-server-cmd "bin/server.bat")
 '(ensime-default-server-root "c:/scala")
 '(hobo-after-save-transmit (quote auto))
 '(hobo-alias-alist (quote (("jhu" "jgraehl" "login.clsp.jhu.edu" "" "") ("isi" "graehl" "nlg0.isi.edu" "" ""))))
 '(hobo-default-alias "jhu")
 '(hobo-mangle-ala-cygwin-under-win32 t)
 '(hobo-use-agent nil)
 '(load-home-init-file t t)
 '(magit-repo-dirs (quote ("~/cdec" "~")))
 '(mode-compile-always-save-buffer-p t)
 '(mode-compile-reading-time 0)
 '(mouse-wheel-scroll-amount (quote (10 ((shift) . 1))))
 '(safe-local-variable-values (quote ((TeX-master . t))))
 '(w32-symlinks-handle-shortcuts t))

;(load "files.el")
(custom-set-faces
  ;; custom-set-faces was added by Custom.
  ;; If you edit it by hand, you could mess it up, so be careful.
  ;; Your init file should contain only one such instance.
  ;; If there is more than one, they won't work right.
 )

;; custom-set-faces was added by Custom -- don't edit or cut/paste it!
;; Your init file should contain only one such instance.
                                        ; '(default ((t (:stipple nil :background "Black" :foreground "AntiqueWhite" :inverse-video nil :box nil :strike-through nil :overline nil :underline nil :slant normal :weight normal :height 108 :width normal :family "fixed")))))

;expand-file-name

(defun gnu-font-sz (font sz) (interactive)
(if on-emacs
    (if emacs23
        (set-frame-font (concat font "-" sz)
    (condition-case nil
;        (progn
          (let ((fontpre (concat "-*-" font "-"))
                (fontpost (concat "-*-*-" sz "-*-*-*-c-*-*-ansi-")))
          (set-default-font "-*-Verdana-normal-r-*-*-12-*-*-*-c-*-*-ansi-")
          (set-face-font 'italic "-*-Verdana-normal-i-*-*-12-*-*-*-c-*-*-ansi-")
          (set-face-font 'bold-italic "-*-Verdana-bold-i-*-*-12-*-*-*-c-*-*-ansi-")
          (set-default-font (concat (concat fontpre "normal-r") fontpost))
          (set-face-font 'italic (concat (concat fontpre "normal-i") fontpost))
          (set-face-font 'bold-italic (concat (concat fontpre "bold-i") fontpost))
                                        ;          (set-face-font 'italic "-*-Lucida Console-normal-i-*-*-12-96-96-96-c-*-*-1")
                                        ;          (set-face-font 'bold-italic "-*-Lucida Console-bold-i-*-*-12-96-96-96-c-*-*-1")
                                        ;       "-*-Proggy Clean-normal-r-*-*-12-*-*-*-c-*-*-ansi-"
                                        ;"-*-Proggy Square-normal-r-*-*-12-*-*-*-c-*-*-ansi-"

          )
      (error nil))))))
(defun gnu-font (font) (interactive)
  (gnu-font-sz font "10"))
;(gnu-font "Verdana")
;(gnu-font "Lucida Console")
;(gnu-font "Courier New")
(and on-win32 (gnu-font "Bitstream Vera Sans Mono"))
(and on-win32
;(gnu-font "Inconsolata-dz")
     (gnu-font "Droid Sans Mono")
(gnu-font "Consolas")
)
;(gnu-font "Anonymous Pro")
;(gnu-font "Georgia")
;; --------------------------------------------------------------------
;; Environment and paths.
;; --------------------------------------------------------------------
                                        (cond (on-unix
                                               (defvar autosave-directory (expand-file-name "~/dot/.emacs.saves"))))
                                        (cond (on-win32
                                               (defvar autosave-directory (expand-file-name "/cache/.emacs.saves"))))

                                        ;(set-foreground-color "white")
                                        ;(set-background-color "Black")
                                        ;"AntiqueWhite"
;(set-face-background 'default "gray90")
;(set-face-background 'default "white")
(set-face-background 'default "white")
(set-face-foreground 'default "black")
;(setq initial-frame-alist      '(        (top . 0) (left . 0)        (width . 230) (height . 76)        (background-color . "white")        (foreground-color . "Black")        (cursor-color   . "red3")        ))


(setq fixed-font "Bitstream Vera Sans Mono")
(setq fixed-face (concat fixed-font ":Regular:9"))
(setq fixed-face-bold (concat fixed-font ":Bold:9"))
(setq prop-font "Verdana")
(setq prop-face (concat prop-font ":Regular:9"))

;(add-hook 'shell-mode-hook 'my-shell-fonts)
;(add-hook 'shell-mode-hook 'install-shell-fonts)


; doesn't work - still using variable width char metrics :(
(defun my-shell-fonts ()
                        (set-face-font 'shell-prompt fixed-face-bold)
                        (set-face-font 'shell-input prop-face)
                        (set-face-font 'shell-output fixed-face)
)

(cond (on-xemacs (progn
                                        ;       (push (expand-file-name "~/dot") load-path)
                                        ;       (set-face-font 'default "ProggyClean:Regular:8")
                                        ;       (set-face-font 'default "ProggySquare:Regular:8")
;                        (set-face-font 'default "Verdana:Regular:9")
                        (set-face-font 'default fixed-face)
;                                               (set-face-font 'default "Bitstream Vera Sans:Regular:8")
                        (set-face-font 'modeline "Verdana:Regular:7")
                        )))

;; Q3.2.5 My tty supports color, but XEmacs doesn't use them.
(cond (on-xemacs
       (if (eq 'tty (device-type))
           (set-device-class nil 'color))))

;; Q3.1.6 How can I have the window title area display the full
;; directory/name of the current buffer file and not just the name?
                                        ;(setq frame-title-format      '("%S: "         (buffer-file-name        "%f" (dired-directory dired-directory "%b"))))

(defun replace-gud-break ()
  (defun gud-break (arg)
    "Set breakpoint at current line."
    (interactive "p")
    (gud-call "stop at \"%f\":%l" 'arg)))

(add-hook 'dbx-mode-hook 'replace-gud-break)

(global-set-key (kbd "M-?") 'lookup-dictionary)
;; dictionary.com
(defun lookup-dictionary (start end)
  "Lookup the selected region at dictionary.com"
  (interactive "r")
  (let ((str (replace-regexp-in-string " " "%20" (buffer-substring start end))))
    (browse-url (concat "http://www.dictionary.com/cgi-bin/dict.pl?term="
                        str))))

(cond (on-emacs (transient-mark-mode t)))



(setq load-path (cons "~/elisp" load-path))
(setq load-path (cons "~/elisp/tuareg-mode" load-path))

;(or recent-emacs
;    (require 'compile-)


;    (require 'compile+)

;    )
;(require 'buffer-move)
;(global-set-key (kbd "<C-S-up>")     'buf-move-up)
;(global-set-key (kbd "<C-S-down>")   'buf-move-down)
;(global-set-key (kbd "<C-S-left>")   'buf-move-left)
;(global-set-key (kbd "<C-S-right>")  'buf-move-right)
;(load "highlight-compile-errors.el")
(load "sourcepair.el")

(and cygwin-xemacs (load "field.el"))
(and cygwin-xemacs (load "env.el"))
(require 'bs)
;(or recent-emacs
(require 'rsh-gud)
(put 'narrow-to-region 'disabled nil)
                                        ;(load-file "~/elisp/tuareg-mode/tuareg.el")
                                        ;(load-file "~/elisp/html-helper-mode.el")
(load-file "~/elisp/tempo.el")
(load-file "~/elisp/re-builder.el")
;(load-file "~/elisp/live-mode.el")

(autoload 'html-helper-mode "html-helper-mode" "Yay HTML" t)
(setq auto-mode-alist (cons '("\\.html$" . html-helper-mode) auto-mode-alist))

(setq inhibit-startup-message t)
(setq inhibit-startup-echo-area-message "zippo")

(setq-default fill-column 80)


(defun set-mark-and-goto-line (line)
  "Set mark and prompt for a line to go to."
  (interactive "NLine #: ")
  (push-mark nil t nil)
  (goto-line line)
  (message "Mark set where you came from."))
(setq default-major-mode 'text-mode)
                                        ;(setq text-mode-hook 'turn-on-auto-fill)
(setq initial-major-mode
      (function (lambda ()
                  (text-mode)
                                        ;        (turn-on-auto-fill)
                  )))
                                        ;(setq semantic-load-turn-everything-on t)
                                        ;(require 'semantic-load)

(setq indent-line-function 'indent-relative-maybe)
(add-hook 'emacs-lisp-mode-hook 'turn-on-font-lock)
(add-hook 'dired-mode-hook 'turn-on-font-lock)
(turn-on-font-lock)

(line-number-mode t)

                                        ; Add time stamp capabilities. See time-stamp.el for doc.

;; A function to insert the time stamp at point.
(defun stamp ()
  "Insert at point the dummy time stamp string to activate the time stamp facility."
  (interactive "*")
  (insert "Time-stamp: <>")             ;insert the bare bones
  (time-stamp)                          ;call the function to fill it in
                                        ;where we put it.
  )

;; enable narrowing w/out a prompt
(put 'narrow-to-region 'disabled nil)


  ;;; Set the default font and frame size for the initial frame.
                                        ;    (font . "-*-vt100-r-*-*-*-*-100-*-*-*-*-iso8859-1")

                                        ;             (background-color . "Gray95")
                                        ;             (foreground-color . "Black")
                                        ;             (background-color . "light blue")
                                        ;             (foreground-color . "navy")
                                        ;             (cursor-color     . "red3")
                                        ;             ))

                                        ;(set-scroll-bar-mode nil)
                                        ; or left or right

                                        ;    (set-face-font 'default "vt100:Regular:10")
                                        ;(set-face-font 'default "Verdana:Regular:10")
                                        ;(set-face-font 'default "ProFontWindows:Regular:8::Western")

                                        ; BUG: is it iso8601 or 8603????
(defvar iso-8601-date-format "%Y-%m-%d"  "Date format specified by ISO 8601.")
(defun insert-iso-8601-date ()
  "Insert today's date in the current buffer. Dates are in ISO 8601 form `1960-10-15'."
  (interactive)
  (insert (format-time-string iso-8601-date-format)))

(defvar rfc-822-date-format "%d %b %Y"  "Date format specified by RFC 822.")
(defun insert-rfc-822-date ()
  "Insert today's date in the current buffer. Dates are in RFC 822 form `15 May 1960'."
  (interactive)
  (insert (format-time-string rfc-822-date-format)))


(defvar rfc-822-datetime-format "%a, %d %b %Y %H:%M:%S %z"  "Date format specified by RFC 822.")
                                        ; ARGL, xemacs hates me :(
                                        ;                                                   ^^^^^^^^^
(defun insert-rfc-822-datetime ()
  "Insert today's date in the current buffer. Dates are in RFC 822 form `Sun, 15 May 1960 15:23:14 CET'."
  (interactive)
  (insert (format-time-string rfc-822-datetime-format)))



(defun insert-date ()
  "Insert today's date in the current buffer. Dates are in form specified by date-format."
  (interactive)
  (insert (format-time-string date-format)))

(defvar date-format iso-8601-date-format "Date format sed by `insert-iso-8601-date'.")


(setq indent-tabs-mode nil)
(defun java-mode-untabify ()
  (save-excursion
    (goto-char (point-min))
    (while (re-search-forward "[ \t]+$" nil t)
      (delete-region (match-beginning 0) (match-end 0)))
    (goto-char (point-min))
    (if (search-forward "\t" nil t)
        (untabify (1- (point)) (point-max))))
  nil)

(and nil
     (add-hook 'c++-mode-hook
          '(lambda ()
             (make-local-variable 'write-contents-hooks)
             (add-hook 'write-contents-hooks 'java-mode-untabify)
             ))
     )

(setq backup-by-copying-when-mismatch t)

;; Treat 'y' or <CR> as yes, 'n' as no.
(fset 'yes-or-no-p 'y-or-n-p)
(define-key query-replace-map [return] 'act)
;(define-key query-replace-map [\C-m] 'act)

(setq minibuffer-max-depth nil)
                                        ;(require 'tex-site)
(setq ecb-auto-activate t)
(defun
 load-ecb ()
  "load ecb"
  (interactive)
  (require 'ecb)
  (ecb-activate)
  )


(require 'font-lock)
                                        ;(if (and (boundp 'window-system) window-system)
                                        ;    (set-face-font 'default "Lucida Console:Regular:10"))

(setq auto-mode-alist
      (append '(("\\.C$"  . c++-mode)
                ("\\.cc$" . c++-mode)
                ("\\.hh$" . c++-mode)
                ("\\.c$"  . c++-mode)
                ("\\.h$"  . c++-mode)
                ("\\.java$" . java-mode) ;jde-mode
                ("\\.bmf$"  . java-mode)
                ("\\.rvw$"  . java-mode)
                ("\\,pl$" . perl-mode)
                )
              auto-mode-alist))

(setq completion-ignored-extensions
      (append '(".tar" ".gz" ".tar.gz" ".tar.Z" ".dvi" ".ps" ".o" ".class")
              completion-ignored-extensions))

(setq c-recognize-knr-p nil)

(setq-default user-full-name "Jonathan Graehl")

(setq dabbrev-backward-only nil)

(defun c-snug-do-if (syntax pos)
  "Dynamically calculate brace hanginess for do-while statements."
  (save-excursion
    (let (langelem)
      (if (and (eq syntax 'block-close)
               (setq langelem (assq 'block-close c-syntactic-context))
               (progn (goto-char (cdr langelem))
                      (if (= (following-char) ?{)
                          (forward-sexp -1))
                      (or (looking-at "\\<do\\>[^_]")
                          1             ;(looking-at "if[^_]")
                          )))
          '()
        '(after)))))



(cond (on-xemacs
       (require 'func-menu)
       (define-key global-map 'f8 'function-menu)
       (add-hook 'find-file-hooks 'fume-add-menubar-entry)
       (define-key global-map "\C-cl" 'fume-list-functions)
       (define-key global-map "\C-cg" 'fume-prompt-function-goto)


       ;; The Hyperbole information manager package uses (shift button2) and
       ;; (shift button3) to provide context-sensitive mouse keys.  If you
       ;; use this next binding, it will conflict with Hyperbole's setup.
       ;; Choose another mouse key if you use Hyperbole.
       (define-key global-map '(shift button3) 'mouse-function-menu)


       ;; For descriptions of the following user-customizable variables,
       ;; type C-h v <variable>
       (setq fume-max-items 25
             fume-fn-window-position 3
             fume-auto-position-popup t
             fume-display-in-modeline-p t
             fume-menubar-menu-location "File"
             fume-buffer-name "*Function List*"
             fume-no-prompt-on-valid-default nil)
       ))

(autoload 'live-mode "live-mode" "live mode" t)

(setq auto-mode-alist (cons '("_log" . live_mode) (cons '("\\.log$" . live-mode) auto-mode-alist)))

(autoload 'cvs-update "pcl-cvs" nil t)
(setq cvs-auto-remove-handled t)
                                        ; (setq cvs-program "cvs")
                                        ; (setq cvs-diff-program "diff")
                                        ; (setq cvs-rmdir-program "rm")

(setq auto-mode-alist (cons '("\\.xml$" . sgml-mode) auto-mode-alist))


(setq font-lock-maximum-decoration t)

                                        ;
                                        ;(defun recompile ()
                                        ;  (interactive)
                                        ;  (compile compile-command))

;; Perl
(require 'cperl-mode)
(defun cperl-mode-tweaks ()
  (auto-fill-mode t)
  (setq cperl-indent-level 4)
  (setq cperl-hairy nil)
  (setq show-trailing-whitespace nil)
;  (local-set-key [{] 'my-electric-braces)
;  (local-set-key [?\M-{] "\C-q{")
;  (local-set-key [(control ?{)] 'my-empty-braces)
;  (local-set-key [;] 'cperl-electric-semi)
;  (setq parens-require-spaces nil)
;  (setq cperl-auto-newline 1)
;  (cperl-toggle-electric 1)
  (setq cperl-font-lock 1)
  (setq cperl-electric-parens nil)
;  (setq cperl-invalid-face (quote off))
;    (setq cperl-highlight-variables-indiscriminately t)
  )
(add-hook 'cperl-mode-hook 'cperl-mode-tweaks)


                                        ;(setq explicit-sh-args '("--login"))
;(or recent-emacs
(progn
(require 'autoinsert)
(add-hook 'find-file-hooks 'auto-insert) ;; enable auto-insert
(setq auto-insert-query nil) ;; don't ask me -- just do it.
(require 'tempo)
)

(defun insert-time ()
  (interactive)
  (insert (current-time-string)))




(global-set-key [(control c) i] 'indent-region)
(global-set-key [(control f5)] 'rotate-eol-coding)


(global-set-key [(meta r)] 'replace-string)
(global-set-key "\C-\\" 'set-mark-and-goto-line)

                                        ;(global-set-key "\M-\?"       'insert-iso-8601-date)
                                        ;(global-set-key "\M-["       'insert-rfc-822-date)
                                        ;(global-set-key "\M-]"       'insert-rfc-822-datetime)
(global-set-key "\M-\C-h" 'my-backward-kill-word)
(global-set-key [(control backspace)] 'my-backward-kill-word)
(global-set-key [(meta backspace)] 'kill-word)
(global-set-key "\M-\C-r" 'query-replace)
(global-set-key "\C-]" 'recompile)
(global-set-key [(control ?')] 'next-error)
(global-set-key [(control leftbracket)] 'next-error)

                                        ;(modify-coding-system-alist 'file "\\.sjs\\'" 'shift_jis)

                                        ;(global-set-key [(control tab)] "\C-q\t")   ; Control tab quotes a tab.
                                        ;(global-set-key [(control ?,)] 'load-ecb)

(global-set-key "\M-o" 'switch-to-other-buffer)
(global-set-key "\C-o" 'other-window)

(global-set-key "\M-/" 'dabbrev-expand)

(add-hook 'noweb-select-mode-hook
          '(lambda () (hack-local-variables-prop-line)))

(add-hook 'tuareg-mode-hook
          '(lambda ()
             (define-key tuareg-mode-map "\M-\C-h" 'tuareg-eval-phrase)
                                        ; your customization code
             ))

;;; delete auto-save files when buffer is explicitly saved
(setq delete-auto-save-files t)


(defun same-buffer-other-window ()
  "switch to the current buffer in the other window"
  (interactive)
  (switch-to-buffer-other-window (current-buffer))
  )
(global-set-key "\M-\C-o" 'same-buffer-other-window)

(setq auto-save-default t)              ; Yes auto save good
(setq auto-save-interval 1000)         ; Number of input chars between auto-saves
(setq auto-save-timeout 3000)      ; Number of seconds idle time before auto-save
(setq backup-by-copying t)              ; don't clobber symlinks
;(setq backup-directory-alist '(("." . "~/.backups"))) ; don't litter my fs tree
(setq delete-old-versions t)            ; clean up a little
(setq kept-new-versions 6)              ; keep 6 new
(setq kept-old-versions 2)              ; keep only 2 old
(setq version-control t)                ; use versioned backups
(defun current-date-and-time ()
  "Insert the current date and time (as given by UNIX date) at dot."
  (interactive)
  (call-process "date" nil t nil))
(global-set-key "\C-x\C-d" 'current-date-and-time)
(and on-win32-emacs (add-untranslated-filesystem "Z:"))
(and on-win32-emacs (add-untranslated-filesystem "/"))
(and on-win32-emacs (add-untranslated-filesystem "~"))

(global-set-key [home] 'beginning-of-buffer)
(global-set-key [end] 'end-of-buffer)
(setq require-final-newline t)
                                        ;(setq display-time-24hr-format t)
                                        ;(display-time)

(setq truncate-lines nil)
(setq truncate-partial-width-windows nil)

(setq process-coding-system-alist '(("bash" . undecided-unix)))
(defun switch-to-other-buffer ()
  "Switch to other-buffer in current window"
  (interactive)
  (switch-to-buffer (other-buffer)))

                                        ;                  (set-face-foreground 'font-lock-type-face "cyan4")
                                        ;(set-face-foreground 'font-lock-builtin-face "Purple")
(set-face-foreground 'font-lock-comment-face "blue")
(set-face-foreground 'font-lock-type-face "steelblue")
                                        ;          (set-face-foreground 'font-lock-constant-face "chocolate")
                                        ;          (set-face-foreground 'font-lock-constant-face "cadetblue")
                                        ;          (set-face-foreground 'font-lock-keyword-face "DarkGoldenRod")
(set-face-foreground 'font-lock-keyword-face "red")
(set-face-foreground 'font-lock-string-face "green4")
(set-face-foreground 'font-lock-function-name-face "red")
(set-face-foreground 'font-lock-variable-name-face "magenta4")


;; Automatically reload files after they've been modified
;; (typically in Visual C++)
(or on-xemacs (progn (require 'autorevert)
(global-auto-revert-mode 1)
))
(setq font-lock-maximum-size 512000)

;; Use the "electric-buffer-list" instead of "buffer-list"
;; so that the cursor automatically switches to the other window
(global-set-key "\C-x\C-b" 'electric-buffer-list)

(and on-emacs (progn
(require 'uniquify)
(setq uniquify-buffer-name-style 'forward)
))

;(load-library "paren")
(and  on-emacs (show-paren-mode t))
(setq show-paren-style 'parenthesis)
(setq bs-cycle-configuration-name "files") ; buffer cycling only between files
(global-set-key "\C-x\C-b" 'bs-show)


(global-set-key [f1] 'help-command)
(global-set-key [f2] 'kill-ring-save)
;(global-set-key [f3] 'kill-region)
;(global-set-key [f4] 'yank)

(global-set-key [f5] 'kill-this-buffer)
(global-set-key [f7] 'save-buffer)
;(global-set-key [f8] 'gdb)

(global-set-key (kbd "M-n") 'grep-buffers)
(global-set-key (kbd "C-M-n") 'rgrep)
;(global-set-key [f9] 'grep)
;(setq grep-command ". ~/.bashrc; egrieg /nfs/topaz/graehl/t/grep-source.sh axy; cat")
                                        ; (global-set-key [f10] 'new-frame)
(require 'dabbrev)
(require 'shell)
(global-set-key [f11] 'shell)
;(global-set-key [(control f11)] 'vc-next-action)

                                        ; (global-set-key [f12] 'compile)

                                        ;(global-set-key [f9] 'remotedebug)

                                        ;(setq iswitchb-default-method 'display)

(if on-emacs (progn
(add-to-list 'auto-coding-regexp-alist '("^\377\376" . utf-16-le) t)
(add-to-list 'auto-coding-regexp-alist '("^\376\377" . utf-16-be) t)
))

(setq dabbrev-case-fold-search nil)



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Dave Abrahams' .emacs file. Changelog at bottom.
;;
;; Instructions for installation:
;;
;; 1. On Windows systems your home directory is the value of the environment
;; variable HOME or failing that, the root directory of your C drive (C:/)
;;
;; 2. This file (.emacs) should be placed in your home directory (Windows users
;; see item 1).
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(setq w32-enable-synthesized-fonts t)


;(condition-case nil
;    (require 'gnus-load)
;  (error nil))

;(or recent-emacs (progn
(require 'advice)
;; on Windoze systems, filenames are not case-sensitive. If we don't arrange for
;; them to come out lower-case when listed, useful toys like ediff-directories
;; won't work properly.
(if (eq system-type 'windows-nt)
    ;; Hook the directory-files function.
    (progn
      (defadvice directory-files (after my-directory-files-advice activate compile)
        "downcases the filenames that result from the builtin directory-files function"
        (setq ad-return-value (mapcar 'downcase ad-return-value)))

;;       (defadvice ediff-files3 (before my-ediff3-forward-slashes activate)
;;         (ad-set-arg 0 (my-forward-slashes (ad-get-arg 0)) t)
;;         (ad-set-arg 1 (my-forward-slashes (ad-get-arg 1)) t)
;;         (ad-set-arg 2 (my-forward-slashes (ad-get-arg 2)) t)
;;         )

;;       (defadvice ediff-temp-file (after my-ediff-temp-file-forward-slashes activate compile)
;;         (setq ad-return-value (my-forward-slashes ad-return-value))
;;         )

      (defun w32-restore-frame ()
        "Restore a minimized frame"
        (interactive)
        ;(w32-send-sys-command 61728)
        )

      (defun w32-maximize-frame ()
        "Maximize the current frame"
        (interactive)
        ;(w32-send-sys-command 61488)
        )

      (if on-win32-emacs
          (w32-maximize-frame))

      ))
;))

(defun my-forward-slashes (filename)
   "convert each backslash in filename to a forward slash"
   (concat (mapcar (function (lambda (c)
                               (if (= c ?\\) ?/ c)))
                   filename)))



;; Setting your default font
;;
;; To set the font for a single session, use shift-left-mouse-button (S-down-mouse-1)
;; To set the font for all sessions, modify the shortcut that starts emacs to use
;; an argument of the form -font "fontname"
;;
;; To find the nasty string to use as a fontname, put the cursor on the '*' in the
;; following line:
;;      (w32-select-font) *
;; type C-xC-e to evaluate the lisp expression. Make a choice in the resulting
;; dialog, and C-x-b *Messages* to switch to the *Messages* buffer. Copy the line
;; you see near the bottom which looks like this:
;;   "-*-Lucida Console-normal-r-*-*-13-97-96-96-c-*-iso8859-1"
;; that's what you want to paste into the shortcut properties dialog after '-font'.


;; I like a slightly gray background for editing on my raster displays
;; contrast tends to work better on my laptop, known as "MOREPIE"
;;
;; To see a list of color names, use control-middle-mouse-button (C-down-mouse-2)
;; and choose "Display Colors" from the resulting menu.

;; Changed my mind about this
;; (if (not (equal (getenv "COMPUTERNAME") "MOREPIE"))
;;    (set-background-color "gray85")

; (set-background-color "white")

;;  )

;;
;; Override some of the default version-control behavior
;;
;(or recent-emacs (progn
;(require 'vc-hooks)

(defun my-force-writable ()
  "Make this buffer and its file writable.  Has no effect on
  buffers not associated with a file"

  (interactive)

  (let ((f (buffer-file-name)))
    (if f
        (let* ((modes (file-modes f))
               (newmodes (logior ?\200 modes))
               )

          (if (not (equal modes newmodes))
              (progn
                (set-file-modes f newmodes)

                (if (not (buffer-modified-p))
                    (revert-buffer nil t t))
                ))
          ))))

;;
;; Set defaults that will be picked up by various modes and other emacs packages.
;;
(setq-default

 ;; "Electrify" a few keys in SGML-mode for editing HTML documents
 sgml-quick-keys t
  sgml-validate-command "tidy"
; sgml-validate-command "tidy -i -wrap 78 --keep-time 0 --gnu-emacs 1 --gnu-emacs-file"

 ;; Tell me if I use M-x <command-name> when there was a key binding for it
 teach-extended-commands-p t

 ;; Non-nil means truncate lines in all windows less than full frame wide.
 truncate-partial-width-windows nil

 ;; Show the file name in the buffer's mode-line
 mode-line-buffer-identification '("%12b [%f]")

 ;; Always indent using spaces instead of tabs (hooked separately for makefiles below)
 indent-tabs-mode nil

 ;; By default, ediff all in one frame.
 ediff-window-setup-function 'ediff-setup-windows-plain

 ;; view diffs side-by-side
 ediff-split-window-function 'split-window-horizontally

 ;; only highlight the selected diff (keeps down gray cruft onscreen)
 ediff-highlight-all-diffs nil

 ;; don't try to use pkunzip to extract
 archive-zip-use-pkzip nil

 mail-personal-alias-file "~/.mailrc"
 )

(if on-emacs
    (progn
      ;; Automatically revert unmodified buffers when they change out from under us on disk.
      (global-auto-revert-mode 1)

      ;; If you don't set hscroll-global-mode, emacs will sometimes prevent you from navigating
      ;; to parts of truncated lines which are off the right side of the window (pane). I find
      ;; this incredibly annoying, so I turn it off.
;      (hscroll-global-mode 1)

      ;; This highlights the region (between point and mark) whenever the mark is
      ;; active. It also causes the mark to be able to become inactive (e.g. by
      ;; typing C-g. To get the mark back, just type C-x C-x.
      (transient-mark-mode 1)
))

;; Enable these two supposedly "advanced" commands which come disabled by default.
(put 'upcase-region 'disabled nil)
(put 'downcase-region 'disabled nil)

(setq my-emacs-version
      (if (string-match "\\([0-9]+[.][0-9]+\\)" (emacs-version))
          (string-to-number (match-string 1 (emacs-version)))))


(if (and on-win32 (< my-emacs-version 21.3))
    (custom-set-faces
     '(bold ((t (:weight bold :height 0.99 :family "tahoma"))))
     '(italic ((t (:slant italic :family "arial"))))
     '(bold-italic ((t (:slant italic :weight bold :family "arial"))))
))

;; This makes it so yanked/typed text replaces any active selection
;; In XEmacs it is aliased to pending-delete-mode
(setq my-ok on-emacs)
(and my-ok (delete-selection-mode 1))

;; For some reason, XEmacs doesn't let newline insertion do pending
;; delete by default.  The way its pending-delete-mode works, it
;; checks each command symbol executed to see if it has the
;; 'pending-delete property, so simply adding the property to the
;; commands in question gets us where we want to be.
(cond ((not on-emacs)
       (put 'newline 'pending-delete t)
       (put 'newline-and-indent 'pending-delete t))
  )

;; Add directories to the path that emacs uses to load its packages
;(setq load-path (cons "~/elisp" load-path))
;(setq load-path (cons "~/elisp/tramp/lisp/" load-path))

(and nil (cond
 ((eq system-type 'windows-nt)  ;; on-win32 -- only an older emacs for cygwin
  (let ((root (if (eq system-type 'windows-nt) "c:/" "/cygdrive/c/")))
;    (setq-default Info-additional-directory-list
;                  (list
;                   (concat root "src/emacs/info")
;                   (concat root "src/gnus/texi")
;                   ))
        (setq load-path
              (append
               (if on-emacs
                   (list ;; (concat root "src/cc-mode")
                         (concat root "src/gnus/lisp"))
                   (list (concat root "src/xcc-mode")
                         (concat root "src/xgnus/lisp"))
                 )
               load-path
               )
              )
      ))))

;; stuff that comes from elsewhere which may get overridden.  At the
;; moment this is just unofficial gnus stuff like ssl.el
;(setq load-path (append load-path '("~/elisp/unofficial/")))

; (require 'tramp)

;(setq load-path (cons "~/elisp/w3" load-path))
; (require 'w3-auto)

;; add some directories for backwards compatibility, depending on the emacs version
(if my-emacs-version
    (mapc (lambda (dir)
              (if (and (string-match "pre-\\([0-9]+[.][0-9]+\\)" dir)
                       (< my-emacs-version (string-to-number (match-string 1 dir))))
                  (setq load-path (cons (concat "~/elisp/" dir) load-path))))
            (directory-files "~/elisp" nil "pre-\\([0-9]+[.][0-9]+\\)")))

;;; stuff from Brad

;(autoload 'speedbar-frame-mode "speedbar" "Popup a speedbar frame" t)
;(autoload 'speedbar-get-focus  "speedbar" "Jump to speedbar frame" t)

; Add a menu entry called Tools where speedbar can live
(if on-emacs
    (define-key-after (lookup-key global-map [menu-bar tools])
      [speedbar] '("Speedbar" . speedbar-frame-mode) [calendar]))

;     (setq send-mail-function 'feedmail-send-it)
;     (autoload 'feedmail-send-it "feedmail")

;(condition-case nil  (load-file "~/auth-credentials.el")  (error nil))

(if (not on-emacs)
    (setq-default message-mail-alias-type nil))
(setq tab-width 4)
(setq-default ;; fill-column 80 ;;;;; Maybe narrower is better
      tab-width 4
      next-line-add-newlines nil
      require-final-newline nil

      Shell-command-switch "-c"

      ;; Mail settings
;      user-full-name "David Abrahams"
;      user-mail-address "dave@boost-consulting.com"
      mail-user-agent 'message-user-agent

      send-mail-function 'smtpmail-send-it
      message-send-mail-function 'smtpmail-send-it
 ;     message-subject-re-regexp "^[    ]*\\(\\([Rr][Ee]\\|[Aa][Ww]\\)\\(\\[[0-9]*\\]\\)*:[     ]*\\)*[         ]*"
      message-syntax-checks '((sender . disabled) (long-lines . disabled))
      smtpmail-code-conv-from nil

      smtpmail-debug-info t ;; make a trace of all SMTP transactions


      ;;
      ;; Printer Settings
      ;;
;      ps-printer-name "\\\\ab-east1\\abe_print1"
;      printer-name "\\\\ab-east1\\abe_print1"

      ps-landscape-mode t
      ps-font-family 'Courier
      ps-font-size 7.0
      ps-inter-column 18
      ps-top-margin 30
      ps-left-margin 18
      ps-right-margin 18
      ps-bottom-margin 18
      ps-header-pad 9
      ps-header-line-pad 0
      ps-header-font-size 9.0
      ps-header-title-font-size 10
      ps-header-offset 9
      ps-number-of-columns 2
      )
                                        ;                  (set-face-foreground 'font-lock-type-face "cyan4")
                                        ;(set-face-foreground 'font-lock-builtin-face "Purple")
(set-face-foreground 'font-lock-comment-face "blue")
(set-face-foreground 'font-lock-type-face "steelblue")
                                        ;          (set-face-foreground 'font-lock-constant-face "chocolate")
                                        ;          (set-face-foreground 'font-lock-constant-face "cadetblue")
                                        ;          (set-face-foreground 'font-lock-keyword-face "DarkGoldenRod")
(set-face-foreground 'font-lock-keyword-face "red")
(set-face-foreground 'font-lock-string-face "green4")
(set-face-foreground 'font-lock-function-name-face "red")
(set-face-foreground 'font-lock-variable-name-face "magenta4")

(set-face-foreground font-lock-builtin-face "red3")
(set-face-foreground font-lock-comment-face "red3")
(set-face-foreground font-lock-constant-face "slateblue")
(set-face-foreground font-lock-string-face "darkgreen")

;; (if (not on-emacs)  ; I don't know how to use BBDB yet anyway.
;;     (require 'bbdb)
;;   (bbdb-initialize 'gnus 'message 'sc 'w3 'reportmail))

;;
;; Special stuff for NT
;;

(if (eq system-type 'windows-nt)
    (progn
      ;;
      ;; We don't know what this does but Brad swears it helps with NT
      ;;
          (require 'comint)
      (fset 'original-comint-exec-1 (symbol-function 'comint-exec-1))
      (defun comint-exec-1 (name buffer command switches)
        (let ((binary-process-input t)
              (binary-process-output nil))
          (original-comint-exec-1 name buffer command switches)))))

(if on-win32
      ;; Set our common backup file repository
      (setq backup-file-dir (concat (or (getenv "TEMP") win32-tempdir) "/emacs~"))

  ;; else
  (setq backup-file-dir "/tmp/emacs~")
  )
(when on-win32
       (defun my-shell-setup ()
       "For Cygwin bash under Emacs 20"
       (setq comint-scroll-show-maximum-output 'this)
       (make-variable-buffer-local 'comint-completion-addsuffix))
       (setq comint-completion-addsuffix t)
       ;; (setq comint-process-echoes t) ;; reported that this is no longer needed
       (setq comint-eol-on-send t)
       (setq w32-quote-process-args ?\")

     (setq shell-mode-hook 'my-shell-setup)
     )

;;
;; Stuff for dealing with emacs backup files. Instead of littering our
;; directories with them they will go into a common black hole for later
;; disposal.
;;

;; Create the backup file directory if it doesn't exist
(if (not (file-directory-p backup-file-dir))
    (make-directory backup-file-dir))

(defun my-convert-slashes (filename)
  "Look at each character in a string and change ':' to '-' and '/' to '_'"
  (concat (mapcar (function (lambda (c)
                              (cond ((= c ?/) ?_)
                                         ((= c ?:) ?-)
                                         (t c))))
                  filename)))

;;
;; Create name mangler for backup files. Creates one honking file
;; name by morphing path components into the filename. Used to keep
;; backup files in one location without conflicts.
;;
(if (or (not my-emacs-version) (< my-emacs-version 21.1))

    (defun make-backup-file-name (filename)
      "Create a non-numeric backup file name for FILENAME. Convert the file path
into one long file name and places it in the directory given by backup-file-dir."
      (expand-file-name (concat (my-convert-slashes
                                 (expand-file-name (file-name-directory
                                                    filename)))
                                (file-name-nondirectory filename) "~")
                        backup-file-dir))

  (setq-default
   backup-directory-alist (list (cons "." backup-file-dir)))

  )

;; (setq make-backup-file-name-function 'my-make-backup-file-name)


;(or recent-emacs (progn

(defun electric-pair ()
  "Insert character pair without sournding spaces"
  (interactive)
  (let (parens-require-spaces)
    (insert-pair)))

;;))
;;
;; Matlab
;;
;(autoload 'matlab-mode "matlab" "Enter Matlab mode." t)
;(autoload 'matlab-shell "matlab" "Interactive Matlab mode." t)


;;
;; my-compile, my-recompile - easy compilation with scrolling errors, and easy
;;      recompilation without worrying about what buffer you're in.
;;

;; Used by my-compile and my-recompile to get back to the bottom of a
;; compilation buffer after save-excursion brings us back to the place we
;; started.

(defun my-end-of-current-compilation-buffer()
  (if (equal (buffer-name) "*compilation*")
      (bufend)))

(defun my-compile(&optional command)
  (interactive)
  (if (interactive-p)
      (call-interactively 'compile)
    (compile command))
  (save-excursion
    (pop-to-buffer "*compilation*")
    (bufend))
  ;; force scrolling despite save-excursion
  (my-end-of-current-compilation-buffer))

(defun my-buffer-exists (buffer)
  "Return t if the buffer exists.
buffer is either a buffer object or a buffer name"
  (bufferp (get-buffer buffer)))

(defun my-recompile ()
  "Run recompilation but put the point at the *end* of the buffer
so we can watch errors as they come up"
  (interactive)
  (if (and (my-buffer-exists "*compilation*")
           compile-command)
      (save-excursion
        ;; switching to the compilation buffer here causes the compile command to be
        ;; executed from the same directory it originated from.
        (pop-to-buffer "*compilation*")
        (recompile)
        (pop-to-buffer "*compilation*")
        (bufend))
    ;; else
    (call-interactively 'my-compile))
  ;; force scrolling despite save-excursion
  (my-end-of-current-compilation-buffer))

;;
;; TLM (version-control) utilities
;;

; (defun my-tlm-diff-latest()
;   "run TLM diff with the current buffer against the latest version under version control."
;   (interactive)
;   (let ((file-name (file-name-nondirectory (buffer-file-name))))
;     (let ((temp-file (concat temporary-file-directory file-name)))
;       (shell-command (concat "rm -f " temp-file)) ;; remove any existing temp file
;       (shell-command (concat "tlm get " file-name " * " temp-file))
;       (shell-command (concat "chmod -w " temp-file)) ;; should not be writable
;       (ediff (buffer-file-name) temp-file)
;       (delete-file temp-file))))

;;
;; General utilities
;;

(defun my-kill-buffer ()
  "Just kill the current buffer without asking, unless of course it's a
modified file"
  (interactive)
  (kill-buffer (current-buffer)))

(defun my-switch-to-previous-buffer ()
  "Switch to the most recently visited buffer without asking"
  (interactive)
  (switch-to-buffer nil))

(defun my-info-other-frame ()
  (interactive)
  (select-frame (make-frame))
  (info))


(defun my-matching-paren (arg)
  (interactive "P")
  (if arg
      () ;;(insert "%")  ; insert the character we're bound to
    (cond ((looking-at "[[({]")
           (forward-sexp 1)
           (forward-char -1))
          ((looking-at "[]})]")
           (forward-char 1)
           (forward-sexp -1))
          (t
           ;; (insert "%")  ; insert the character we're bound to
      ))))

; Something for converting DOS files to unix format
(defun my-use-code-undecided-unix ()
  (interactive)
  (set-buffer-file-coding-system 'undecided-unix)
  (save-buffer))

(defun my-other-window-backward (&optional n)
  "Select the previous window. Copied from \"Writing Gnu Emacs Extensions\"."
  (interactive "P")
  (other-window (- (or n 1)))
  )

; return the first non-nil result of applying f to each element of seq
(defun my-first-non-nil (seq f)
;  (message "my-first-non-nil %s %s" seq f)
  (and seq
       (or
        (apply f (list (car seq)))
        (my-first-non-nil (cdr seq) f)))
  )

;;; because xemacs 21.4 is missing this from its files.el
(defun my-file-expand-wildcards (pattern &optional full)
  "Expand wildcard pattern PATTERN.
This returns a list of file names which match the pattern.

If PATTERN is written as an absolute relative file name,
the values are absolute also.

If PATTERN is written as a relative file name, it is interpreted
relative to the current default directory, `default-directory'.
The file names returned are normally also relative to the current
default directory.  However, if FULL is non-nil, they are absolute."
  (let* ((nondir (file-name-nondirectory pattern))
         (dirpart (file-name-directory pattern))
         ;; A list of all dirs that DIRPART specifies.
         ;; This can be more than one dir
         ;; if DIRPART contains wildcards.
         (dirs (if (and dirpart (string-match "[[*?]" dirpart))
                   (mapcar 'file-name-as-directory
                           (file-expand-wildcards (directory-file-name dirpart)))
                 (list dirpart)))
         contents)
    (while dirs
      (when (or (null (car dirs))       ; Possible if DIRPART is not wild.
                (file-directory-p (directory-file-name (car dirs))))
        (let ((this-dir-contents
               ;; Filter out "." and ".."
               (delq nil
                     (mapcar #'(lambda (name)
                                 (unless (string-match "\\`\\.\\.?\\'"
                                                       (file-name-nondirectory name))
                                   name))
                             (directory-files (or (car dirs) ".") full
                                              (wildcard-to-regexp nondir))))))
          (setq contents
                (nconc
                 (if (and (car dirs) (not full))
                     (mapcar (function (lambda (name) (concat (car dirs) name)))
                             this-dir-contents)
                   this-dir-contents)
                 contents))))
      (setq dirs (cdr dirs)))
    contents))


;; Older versions of GNU Emacs (pre-20.6, probably a bit earlier) had an annoying
;; habit of creating new buffers for you if you quickly used TAB RET to auto-complete
;; a buffer name when switching buffers and if there was more than one valid
;; completion. This appears to be fixed now, but these definitions don't seem to
;; interfere and might also work well for XEmacs.
(when nil
(defadvice switch-to-buffer (before my-existing-buffer
                                    activate compile)
  "When switching buffers interactively, only switch to existing buffers
unless given a prefix argument."
  (interactive
   (list (read-buffer "Switch to buffer: "
                      (other-buffer)
                      (null current-prefix-arg)))))

(defadvice switch-to-buffer-other-window (before my-existing-buffer-other-window
                                    activate compile)
  "When switching buffers interactively, only switch to existing buffers
unless given a prefix argument."
  (interactive
   (list (read-buffer "Switch to buffer in other window: "
                      (other-buffer)
                      (null current-prefix-arg)))))

(defadvice switch-to-buffer-other-frame (before my-existing-buffer-other-frame
                                    activate compile)
  "When switching buffers interactively, only switch to existing buffers
unless given a prefix argument."
  (interactive
   (list (read-buffer "Switch to buffer in other frame: "
                      (other-buffer)
                      (null current-prefix-arg)))))
)

;; Emacs has a bunch of built-in commands for working with rectangular regions
;; of the screen (try "M-x apropos RET rectangle" for a list). These can be
;; *really cool* for making diagrams in text. There a couple of really useful
;; things missing from the built-in rectangle support, though, especially if
;; you're making pictures. First, the built-in yank-rectangle moves text which
;; is to the right of point over to avoid the new text. Sometimes you just want
;; that, but other times you just want to drop in a yanked rectangle on top of
;; what's there without disturbing the rest of the picture. That's what
;; my-yank-replace-rectangle does. Also, there's no built-in way of copying a
;; region to the rectangle kill-buffer. For that, we have my-save-rectangle.
(defun my-yank-replace-rectangle ()
  "Replace a rectangular region with the last killed rectangle, placing its upper left corner at point."
  (interactive)
  (my-replace-rectangle killed-rectangle))

(defun my-replace-rectangle (rectangle)
  "Replace rectangular region with RECTANGLE, placing its upper left corner at point.
RECTANGLE's first line is inserted at point, its second
line is inserted at a point vertically under point, etc.
RECTANGLE should be a list of strings.
After this command, the mark is at the upper left corner
and point is at the lower right corner."
  (let (
        (lines rectangle)
        (insertcolumn (current-column))
        (save-overwrite-mode overwrite-mode)
        (width (length (car rectangle)))
        (endcolumn (+ (current-column) (length (car rectangle))))
        (first t))
    (push-mark)
    (setq overwrite-mode nil)
    (while lines
      (or first
          (progn
            (forward-line 1)
            (or (bolp) (insert ?\n))))
      (move-to-column endcolumn)
      (delete-backward-char width)
      (setq first nil)
      (insert (car lines))
      (setq lines (cdr lines)))
    (setq overwrite-mode save-overwrite-mode)
    ))

(defun my-save-rectangle (start end)
  "Save rectangle with corners at point and mark as last killed one.
Calling from program, supply two args START and END, buffer positions."
  (interactive "r")
  (setq killed-rectangle (extract-rectangle start end)))

(defun my-kill-rectangle (start end)
  "Save rectangle with corners at point and mark as last killed one,
and erase it.  Calling from program, supply two args START and END,
buffer positions."
  (interactive "r")
  (my-save-rectangle start end)
  (clear-rectangle start end))

;;;;;;;;;;;;;;;;;;;;;;;;
;;                    ;;
;; Mode Customization ;;
;;                    ;;
;;;;;;;;;;;;;;;;;;;;;;;;

;;
;; dired (directory navigation in the editor)

;; Make it so dired lets you explore directories in a single window (pane) without
;; constantly opening subdirectories and files in new panes. This is the way dired
;; works in XEmacs
(defun dired-mouse-find-file(event)
  "switch to the clicked file or directory in the same window"
  (interactive "e")
  (mouse-set-point event)
  (dired-find-file))

(add-hook 'dired-mode-hook
          '(lambda ()
             (define-key dired-mode-map [(mouse-2)] 'dired-mouse-find-file)
             ))

;; message

(add-hook 'message-mode-hook
          '(lambda () (auto-fill-mode t)))

;;
;; Picture
;;
(defun my-picture-mode-hook ()
  (modify-syntax-entry ?| "w")
  (modify-syntax-entry ?+ "w"))

(add-hook 'picture-mode-hook 'my-picture-mode-hook)


;;
;; html
;;

;; don't insert newlines around <code>...</code> tags
;;; Set up PSGML
; Add PSGML to load-path so Emacs can find it.
; Note the forward slashes in the path... this is platform-independent so I
; would suggest using them over back slashes. If you use back slashes, they
; MUST BE doubled, as Emacs treats backslash as an escape character.
;(setq load-path (cons "~/elisp/psgml-1.3.1/" load-path))
; Use PSGML for sgml and xml major modes.
(when nil
(condition-case nil
    (progn
      (require 'psgml)
      (autoload 'sgml-mode "psgml" "Major mode to edit SGML files." t)
      (autoload 'xml-mode "psgml" "Major mode to edit XML files." t)
      ; override default validate command to utilize OpenSP's onsgmls executable
      (setq sgml-validate-command "onsgmls -s %s %s")
      (add-to-list 'sgml-catalog-files "~/dtd/CATALOG")

      ; override default xml-mode validate command to utilize OpenSP's onsgmls
      ; executable by using a mode-hook, since there appears to be no other means
      ; to accomplish it.
      (defun my-psgml-xml-hook ()
        (setq sgml-validate-command "onsgmls -s %s %s")
        (setq sgml-declaration "c:/cygwin/usr/local/lib/sgml/dtd/html/xml.dcl")
        )
      (add-hook 'xml-mode-hook 'my-psgml-xml-hook)

      (defun my-sgml-electric-less-than (&optional arg)
        (interactive "*P")
        (if arg (insert "<")
          (call-interactively
           (if (is-mark-active)  'sgml-tag-region 'sgml-insert-element))
          ))

      (defun my-psgml-hook ()
      ; From Lennart Staflin - re-enabling launch of browser (from original HTML mode)
        (local-set-key "\C-c\C-b" 'browse-url-of-buffer)
        (local-set-key "<" 'my-sgml-electric-less-than)
        )

      (add-hook 'sgml-mode-hook 'my-psgml-hook)

                                        ; PSGML - enable face settings
        (setq-default sgml-set-face t)

        ; Auto-activate parsing the DTD when a document is loaded.
        ; If this isn't enabled, syntax coloring won't take affect until
        ; you manually invoke "DTD->Parse DTD"
        (setq-default sgml-auto-activate-dtd t)
      )
  (error
   (or recent-emacs (progn (require 'sgml-mode)
   (setq html-tag-alist
         (append
          '( ("code"))
          html-tag-alist)
         )))
   )
  )
)


;;; Set up my "DTD->Insert DTD" menu.

(setq sgml-custom-dtdx '
      (
       ( "BoostBook"
         "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE chapter PUBLIC \"-//Boost//DTD BoostBook XML V1.0//EN\" \"http://www.boost.org/tools/boostbook/dtd/boostbook.dtd\">" )
       ( "XHTML 1.0 Strict"
         "<?xml version=\"1.0\"?>\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"xhtml1-strict.dtd\">" )
       ( "XHTML 1.0 Transitional"
         "<?xml version=\"1.0\"?>\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"xhtml1-transitional.dtd\">" )
       ( "XHTML 1.0 Frameset"
         "<?xml version=\"1.0\"?>\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Frameset//EN\" \"xhtml1-frameset.dtd\">" )
; I use XHTML now! (not)
       ( "HTML 4.01 Transitional"
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">" )
       ( "HTML 4.01 Strict"
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">" )
       ( "HTML 4.01 Frameset"
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\">" )
; An example of IBMIDDoc SGML DTD
;       ( "IBMIDDoc"
;        "<!DOCTYPE ibmiddoc PUBLIC \"+//ISBN 0-933186::IBM//DTD IBMIDDoc//EN\" [\n]>")
       ( "DOCBOOK XML 4.1.2"
        "<?xml version=\"1.0\"?>\n<!DOCTYPE book PUBLIC \"-//OASIS//DTD DocBook XML V4.2//EN\" \"http://www.oasis-open.org/docbook/xml/4.2/docbookx.dtd\" [\n]>")
       )
      )

;;; Set up and enable syntax coloring.
; Create faces to assign markup categories.
(make-face 'sgml-doctype-face)
(make-face 'sgml-pi-face)
(make-face 'sgml-comment-face)
(make-face 'sgml-sgml-face)
(make-face 'sgml-start-tag-face)
(make-face 'sgml-end-tag-face)
(make-face 'sgml-entity-face)
; Assign attributes to faces. Background of white assumed.
(set-face-foreground 'sgml-doctype-face "blue1")
(set-face-foreground 'sgml-sgml-face "cyan1")
(set-face-foreground 'sgml-pi-face "magenta")
(set-face-foreground 'sgml-comment-face "purple")
(set-face-foreground 'sgml-start-tag-face "Red")
(set-face-foreground 'sgml-end-tag-face "Red")
(set-face-foreground 'sgml-entity-face "Blue")
; Assign faces to markup categories.
(setq sgml-markup-faces
'((doctype . sgml-doctype-face)
(pi . sgml-pi-face)
(comment . sgml-comment-face)
(sgml . sgml-sgml-face)
(comment . sgml-comment-face)
(start-tag . sgml-start-tag-face)
(end-tag . sgml-end-tag-face)
(entity . sgml-entity-face)))

; insert entities in lowercase
(setq sgml-entity-insert-case 'lower)

(or recent-xemacs
(define-skeleton html-href-anchor
  "HTML anchor tag with href attribute."
  "URL: "
  "<a href=\"" str "\">" _ "</a>")
)

(defun is-mark-active ()
  "FIXME: gnu emacs return mark-active"
  (if on-xemacs
      (mark)
    mark-active
    )
  )

(defun my-mark-or-point ()
  "Return the mark if it is active, otherwise the point."
  (if (is-mark-active) (mark) (point)))

(defun my-selection ()
  "Return a pair [start . finish) delimiting the current selection"
      (let ((start (make-marker))
            (finish (make-marker)))
        (set-marker start (min (my-mark-or-point) (point)))
        (set-marker finish (max (my-mark-or-point) (point)))
        (cons start finish)))

(defun my-replace-in-region (start finish key replacement)
  "In the range [START, FINISH), replace text matching KEY with REPLACEMENT"
  (goto-char start)
  (while (search-forward key finish t)
    (replace-match replacement)))

(defun my-activate-mark ()
  "Make the mark active if it is currently inactive"
  (set-mark (mark t)))


(defun my-code-tag ()
  "Surround the region with <code>...</code>, formatting it as code."
  (interactive)
  (sgml-tag "code"))

(defun my-sgml-validate-writeback ()
  (interactive)
  (let* ((file (buffer-file-name))
         (cmd
          (format "tidy -m -i -wrap 78 --force-output 1 --keep-time 0 --gnu-emacs 1  %s" file) ))

  (save-some-buffers (not compilation-ask-about-save) nil)
  ;compile-internal
  (compilation-start cmd "No more errors")
;  (sgml-validate cmd)
    ))

(defun my-convert-html-literals ()
  "convert special characters in the region as follows:

   \"&\" => \"&amp;\"     \"<\" => \"&lt;\"      \">\" => \"&gt;\"    \"\\\"\" => \"&quot;\"
This makes a region of source code appear correctly in an HTML file."
  (interactive)
  (save-excursion
    (let ((start (car (my-selection)))
          (finish (cdr (my-selection))))
      (my-replace-in-region start finish "&" "&amp;")
      (my-replace-in-region start finish "<" "&lt;")
      (my-replace-in-region start finish ">" "&gt;")
      (my-replace-in-region start finish "\"" "&quot;")
      )))

(defun my-preformatted ()
  "Surround the region with <pre>...</pre> and convert
special characters contained within as follows:

   \"&\" => \"&amp;\"     \"<\" => \"&lt;\"      \">\" => \"&gt;\"    \"\\\"\" => \"&quot;\"
This makes a region of source code appear correctly in an HTML file."
  (interactive)
        (my-convert-html-literals)
  (sgml-tag "pre")
)

(defun my-yank-code ()
  "Yank whatever was last killed, add HTML formatting as blockquoted,
preformatted text, and translate the special characters \"<\\\">&\" to their HTML
equivalents."
  (interactive)
  (yank)
  (my-activate-mark)
  (my-convert-html-literals))

; workaround for XEmacs
(if (not (boundp 'show-paren-mode))
    (defun show-paren-mode (yes)))

(defun my-code-mode-hook ()
;  (font-lock-mode t)
  (show-paren-mode t)
  (local-set-key [return] 'newline-and-indent)
  (local-set-key [(control return)] 'newline)
  (local-set-key [( control ?\( )] 'my-matching-paren)
  (make-local-variable 'dabbrev-case-fold-search)
  (setq dabbrev-case-fold-search nil)
  )

;;
;; Jam
;;
(require 'jam-mode)

(or recent-xemacs (progn
;(load "eshell/esh-util.el")
(require 'esh-util)
(defun my-jam-electric-semicolon ()
  (interactive "*")
  (insert
   (save-excursion
     (let ((start (point)))
       (if (and (re-search-backward "^[^#]*[^ \t\n]" (line-beginning-position))
                (equal (match-end 0) start))
           " ;" ";")))))
))

(defun my-sh-indentation ()
  (save-excursion
    (set-mark (point)) ; {
    (if (re-search-backward "^[^#\n]*\\(\\[\\|]\\|(\\|)\\|{\\|}\\)" nil t)
        (+ (current-indentation)
           (progn
             (goto-char (match-beginning 1))
             (if (looking-at "[[{(]") 4 0)))
      0)))

(defun my-sh-newline-and-indent ()
  (interactive "*")
  (newline)
  (indent-line-to
   (save-excursion
     (skip-chars-backward " \t\n")
     (+ (my-sh-indentation)
        (let ((start (point)))
          (if (and (re-search-backward "^[^#\n]*[;{}]" (line-beginning-position) t)
                   (equal (match-end 0) start))
              0 4))))))

(defun my-sh-electric-braces ()
  (interactive "*")
  (let ((indentation (my-sh-indentation)))
    (if (equal (current-indentation) (current-column))
        (indent-line-to indentation))
    (insert "{}")
    (backward-char)
    (newline)
    (newline)
    (indent-line-to indentation)
    (prev-line 1)
    (indent-to (+ indentation 4))))

(defun my-sh-electric-open-brace ()
  (interactive "*")
  (let ((indentation (my-sh-indentation)))
    (if (equal (current-indentation) (current-column))
        (indent-line-to indentation))
    (insert "{")
    (newline)
    (indent-line-to (+ indentation 4))))

;; Stolen from lisp-mode.el, with slight modifications for reformatting comments
;;
(defun my-sh-fill-paragraph (&optional justify)
  "Like \\[fill-paragraph], but handle Emacs Lisp comments.
If any of the current line is a comment, fill the comment or the
paragraph of it that point is in, preserving the comment's indentation
and initial semicolons."
  (interactive "P")
  (let (
    ;; Non-nil if the current line contains a comment.
    has-comment

    ;; Non-nil if the current line contains code and a comment.
    has-code-and-comment

    ;; If has-comment, the appropriate fill-prefix for the comment.
    comment-fill-prefix
    )

    ;; Figure out what kind of comment we are looking at.
    (save-excursion
      (beginning-of-line)
      (cond

       ;; A line with nothing but a comment on it?
       ((looking-at "[ \t]*#[# \t]*")
    (setq has-comment t
          comment-fill-prefix (buffer-substring (match-beginning 0)
                            (match-end 0))))

       ;; A line with some code, followed by a comment?  Remember that the
       ;; semi which starts the comment shouldn't be part of a string or
       ;; character.
       ((condition-case nil
        (save-restriction
          (narrow-to-region (point-min)
                (save-excursion (end-of-line) (point)))
          (while (not (looking-at "#\\|$"))
        (skip-chars-forward "^#\n\"\\\\?")
        (cond
         ((eq (char-after (point)) ?\\) (forward-char 2))
         ((memq (char-after (point)) '(?\" ??)) (forward-sexp 1))))
          (looking-at "#+[\t ]*"))
      (error nil))
    (setq has-comment t has-code-and-comment t)
    (setq comment-fill-prefix
          (concat (make-string (/ (current-column) 8) ?\t)
              (make-string (% (current-column) 8) ?\ )
              (buffer-substring (match-beginning 0) (match-end 0)))))))

    (if (not has-comment)
        ;; `paragraph-start' is set here (not in the buffer-local
        ;; variable so that `forward-paragraph' et al work as
        ;; expected) so that filling (doc) strings works sensibly.
        ;; Adding the opening paren to avoid the following sexp being
        ;; filled means that sexps generally aren't filled as normal
        ;; text, which is probably sensible.  The `;' and `:' stop the
        ;; filled para at following comment lines and keywords
        ;; (typically in `defcustom').
    (let ((paragraph-start (concat paragraph-start
                                       "\\|\\s-*[\(#:\"]")))
          (fill-paragraph justify))

      ;; Narrow to include only the comment, and then fill the region.
      (save-excursion
    (save-restriction
      (beginning-of-line)
      (narrow-to-region
       ;; Find the first line we should include in the region to fill.
       (save-excursion
         (while (and (zerop (prev-line 1))
             (looking-at "^[ \t]*#")))
         ;; We may have gone too far.  Go forward again.
         (or (looking-at ".*#")
         (forward-line 1))
         (point))
       ;; Find the beginning of the first line past the region to fill.
       (save-excursion
         (while (progn (forward-line 1)
               (looking-at "^[ \t]*#")))
         (point)))

      ;; Lines with only semicolons on them can be paragraph boundaries.
      (let* ((paragraph-start (concat paragraph-start "\\|[ \t#]*$"))
         (paragraph-separate (concat paragraph-start "\\|[ \t#]*$"))
         (paragraph-ignore-fill-prefix nil)
         (fill-prefix comment-fill-prefix)
         (after-line (if has-code-and-comment
                 (save-excursion
                   (forward-line 1) (point))))
         (end (progn
            (forward-paragraph)
            (or (bolp) (newline 1))
            (point)))
         ;; If this comment starts on a line with code,
         ;; include that like in the filling.
         (beg (progn (backward-paragraph)
                 (if (eq (point) after-line)
                 (prev-line 1))
                 (point))))
        (fill-region-as-paragraph beg end
                      justify nil
                      (save-excursion
                    (goto-char beg)
                    (if (looking-at fill-prefix)
                        nil
                      (re-search-forward comment-start-skip)
                      (point))))))))
    t))
(require 'sh-script)
;; not very useful
(defun my-sh-electric-close-brace ()
  (interactive "*")
  (let ((indentation
        (progn
          (delete-region (point)
                         (progn
                         (or (zerop (skip-chars-backward " \t\n"))
                             (if (sh-quoted-p)
                                 (forward-char)))
                         (point)))
          (if (equal (char-before) 123) (current-indentation)
              (- (current-indentation) 4)))))
    (newline)
    (indent-to indentation)
    (insert "}")
    (newline)
    (indent-to indentation)))


(defun my-jam-debug-mode ()
  (interactive)
;;  (compilation-mode)
  (local-set-key [(control f10)] 'jam-debug-prev)
  (local-set-key [f10] 'jam-debug-next)
  (local-set-key [(shift f11)] 'jam-debug-finish)
  (local-set-key [(control shift f11)] 'jam-debug-caller)
  (local-set-key [f11] 'jam-debug-in))

;;
;; html
;;
(defun my-html-mode-hook ()
  (local-set-key [f7] 'my-sgml-validate-writeback)
;  (local-set-key [\C-f7] 'sgml-validate)
  (local-set-key [(control f7)] 'sgml-validate)
  (local-set-key "\C-c\C-c\C-c" 'my-code-tag)
  (local-set-key "\C-c\C-c\C-q" 'my-preformatted)
  (local-set-key "\C-c\C-cy" 'my-yank-code)
  )

(add-hook 'html-mode-hook 'my-html-mode-hook)

;;
;; sh
;;
(defun my-sh-mode-hook ()
  (interactive "*")
;  (my-code-mode-hook)
;  (auto-fill-mode t)
;  (local-set-key [return] 'my-sh-newline-and-indent)
;  (local-set-key "{" 'my-sh-electric-open-brace)
;  (local-set-key [\S-\M-{] 'my-sh-electric-braces)
;  (setq fill-paragraph-function 'my-sh-fill-paragraph)
  ;; (local-set-key "}" 'my-sh-electric-close-brace)
  )

(add-hook 'sh-mode-hook 'my-sh-mode-hook)

(defun my-my-jam-mode-hook ()
  (interactive "*")
;  (local-set-key ";" 'my-jam-electric-semicolon)
  (auto-fill-mode)
  )

(add-hook 'my-jam-mode-hook 'my-my-jam-mode-hook)

;;
;; Perl
;;
(defun my-perl-mode-hook ()
  (my-code-mode-hook)
  )

(add-hook 'perl-mode-hook 'my-perl-mode-hook)

;;
;; lisp
;;

(defun my-lisp-mode-hook ()
  (my-code-mode-hook)
  )
(add-hook 'lisp-mode-hook 'my-lisp-mode-hook)
(add-hook 'emacs-lisp-mode-hook 'my-lisp-mode-hook)
(add-hook 'lisp-interaction-mode-hook 'my-lisp-mode-hook)

(defun my-unedebug-defun ()
  "I can't believe emacs doesn't give you a way to do this!!"
  (interactive t)
  (eval-expression (edebug-read-top-level-form)))

;;
;; compilation
;;
(defun my-compilation-mode-hook ()
  (setq truncate-lines nil))
                                        ; Don't truncate lines in the compilation window
(add-hook 'compilation-mode-hook 'my-compilation-mode-hook)

;;
;; python
;;



;;
;; restructured text
;;
(defun my-rst-mode-hook ()
  (make-local-variable 'lazy-lock-defer-time)
  (make-local-variable 'lazy-lock-stealth-time)
  (make-local-variable 'lazy-lock-stealth-load)
  (setq lazy-lock-stealth-time 5)
  (setq lazy-lock-defer-time 7)
  (setq lazy-lock-stealth-verbose t)
  (setq lazy-lock-stealth-load 100)

  ;; AWL guidelines say code blocks must be 65 characters wide or
  ;; fewer.  Leave 2 spaces for indent.
  (setq fill-column 67)

  (auto-fill-mode t)
  )

(add-hook 'rst-mode-hook 'my-rst-mode-hook)

;; Customize which modes are automatically invoked on files matching certain
;; patterns.
(setq auto-mode-alist
      (append
       '( ("\\.py$" . python-mode)
          ("\\.lit$" . python-mode)
          ("\\.nlp$" . python-mode)
          ("\\.jam$" . jam-mode)
          ("[Jj]ambase$" . jam-mode)
          ("[Jj]amfile" . jam-mode)
          ("[Jj]amrules$" . jam-mode)
          ("\\..pp$" . c++-mode)
          ("\\.jerr$" . my-jam-debug-mode)
          ("\\.m\\'" . matlab-mode)
          ("\\.rst$" . rst-mode)
          )
       auto-mode-alist))

;(condition-case nil (load-library "rst-mode")  (error nil))


(setq interpreter-mode-alist
      (cons '("python" . python-mode)
            interpreter-mode-alist))

           (autoload 'python-mode "python-mode" "Python editing mode." t)

;;
;; C/C++
;;

(defun my-c-leading-comma-p ()
  (save-excursion
    (beginning-of-line)
    (c-forward-token-2 0 nil (c-point 'eol))
    (eq (char-after) ?,)))

(defun my-c-comma-unindent (langelem)
  "Unindent for leading commas"
  (if (my-c-leading-comma-p) '/))

(defun my-c-comma-indent (langelem)
  "Indent for leading commas"
  (if (my-c-leading-comma-p) '*))

(defun my-cleanup-pp-output ()
  "Clean up preprocessor output so that it's at least semi-readable"
  (interactive)

  (let ((selection (my-selection))
        (start (make-marker))
        (end (make-marker))
        )
    (set-marker start (car selection))
    (set-marker end (cdr selection))

    (c++-mode)
    ;; CR before function declaration id
    (subst-re "\\([a-zA-Z0-9_]\\) +\\([a-zA-Z_][a-zA-Z0-9_]*(\\)" "\\1\n\\2" start end)
    (subst-re "\\(\\<return\\>\\|\\<new\\>\\)\n" "\\1 " start end)

    ;; CR after template parameter list
    (subst-re "\\<template\\> *<\\([^<>]+\\)>" "template <\\1>\n" start end)

    (subst-re " *\\(\\s.\\|[()]\\) *" "\\1" start end)
    (subst-re " +" " " start end)

    (subst-re "\\([{}];*\\)" "\\1\n" start end)  ;
    (subst-re "\\([^ ].*\\)\\([{}]\\)" "\\1\n\\2" start end)

    (subst-re ";\\(.\\)" ";\n\\1" start end)

    (subst-re "\\([(]+\\)\\([(]\\)" "\\1\n\\2" start end)
    (subst-re ">\\(\\<struct\\>\\|\\<class\\>\\)" ">\n\\1" start end)
    (indent-region start end nil)
  ))

(defun my-empty-braces ()
  "insert {  }"
  (interactive "*")
  (insert "{  }")
  (backward-char)
  (backward-char)
  (indent-according-to-mode)
  )

(defun my-electric-braces ()
  "Insert a pair of braces surrounding a blank line, indenting each according to the mode"
  (interactive "*")
  (let ((bolp
         (save-excursion (skip-chars-backward " \t")
                         (equal (current-column) 0))))
    (insert "{}")
    (if bolp
        (eval (list indent-line-function)))
    )
    (backward-char)
    (newline-and-indent)
    (prev-line 1)
    (end-of-line)
    (newline-and-indent))

(setq my-initials "dwa")

(defun boost-copyright ()
  "Return the appropriate boost copyright for the current user and year"
  (concat "Copyright " (user-full-name) " " (number-to-string (nth 5 (decode-time)))
          ". Distributed under the Boost\n\
Software License, Version 1.0. (See accompanying\n\
file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)"))

(defun fluid-copyright ()
  "Return the appropriate FluidObjects copyright for the current user and year"
  (concat "Copyright FluidObjects Software " (number-to-string (nth 5 (decode-time)))
          ". All rights reserved."))

(defun my-split-filename (filename)
  "split a FILENAME string into a (basename . extension) pair"
  (let* ((fname filename)

         (prefix
          (if (string-match "\\(.*\\)\\(\\..*\\)" fname)
              (substring fname 0 (match-end 1))
            fname))
         (extension
          (if (match-beginning 2)
              (substring fname (+ 1 (match-beginning 2)))
            nil)))
    (cons prefix extension)
  ))

(defun my-split-current-filename ()
  (my-split-filename (file-name-nondirectory (buffer-file-name))))

(defun my-include-guard ()
  "Compute the appropriate #include guard based on the current buffer's name"
  (let* ((split-name (my-split-current-filename))
         (time (decode-time))
         (prefix (car split-name))
         (ext (cdr split-name))
         (extension (if ext (concat "_" ext) ""))
         )
    (upcase
     (concat
      prefix "_" my-initials
      (number-to-string (nth 5 time))
      (number-to-string (nth 4 time))
      (number-to-string (nth 3 time)) extension))))

(defun my-split-path (path)
  (let ((result nil) (elt nil))
    (while (and path
                (not (equal (setq elt (file-name-nondirectory path)) "")))
      (setq result (cons elt result))
      (setq path (file-name-directory path))
      (setq path (and path (directory-file-name path))))
    result))

(defun my-add-todo-entry ()
  "like add-change-log-entry but uses filename of TODO"
  (interactive)
  (add-change-log-entry nil "TODO" t t)
  )

(defun my-copyright (&optional copyright)
  "Insert a commented COPYRIGHT string. If COPYRIGHT
is not supplied, the boost copyright is used by default"
  (interactive)
(when nil
  (let ((copy-start (point)))
    (insert (or copyright
                       (or (and (my-path-elts) (boost-copyright))
                           (eval (list my-default-copyright))))
                   "\n")

    (comment-region copy-start (point)))))

(defun my-path-elts ()
  (subseq (my-split-path (buffer-file-name)) 0 -1))

(defcustom my-namespace-roots
      '(("boost". boost-copyright) ("fluid" . fluid-copyright))
      "An alist of root directory names and associated copyright
      functions from which to deduce C++ namespace names."
      ':type 'alist )

(defun my-prepare-source ()
  (let* ((all-path-elts (my-path-elts))

         ;; prune off the head of path-elts up to the last occurrence
         ;; of boost, if any otherwise, path-elts will be nil

         ;; this is the index of the namespace root in the path
         (index (position-if
                   (lambda (x)
                     (find-if
                      (lambda (y) (equal (car y) x))
                      my-namespace-roots))
                   all-path-elts :from-end 0))

         ;; the name of the root element
         (root (and index (nth index all-path-elts)))

         ;; the path elements to use for the namespace
         (top-path-elts (and index (subseq all-path-elts index)))

         (path-elts
          (if (and top-path-elts
                   (equal root "boost")
                   (equal "libs" (cadr top-path-elts))
                   (equal "src" (cadddr top-path-elts)))
              (append (list root (caddr top-path-elts)) (cddddr top-path-elts))
            top-path-elts))

         (copyright-function
          (and index
               (cdr (find-if (lambda (y) (equal (car y) root)) my-namespace-roots))))

         (copyright
          (and index
               (eval (list copyright-function))))

         )
    (cons path-elts copyright)))

(defun my-begin-header ()
  "Begin a C/C++ header with include guards and a copyright."
  (interactive)
  (let* ((guard (my-include-guard))
         (source-prep (my-prepare-source))
         (path-elts (car source-prep))
         (copyright (cdr source-prep)))

    (bufstart)
    (if copyright
        (my-copyright copyright)
      (my-copyright))

    (insert "#ifndef " guard "\n"
                   "# define " guard "\n")

    (let ((final nil) ;; final position
          (nsfini (if path-elts "\n" "")))

      ;; opening namespace stuff
      (insert nsfini)
      (mapc (lambda (n) (insert "namespace " n " { "))
            path-elts)
      (insert nsfini)

      (setq final (point))
      (newline)

      (bufend)
      ;; make sure the next stuff goes on its own line
      (if (not (equal (current-column) 0))
          (newline))

      ;; closing namespace stuff
      (mapc (lambda (n) (insert "}")) path-elts)
      (reduce (lambda (prefix n)
                (insert prefix n) "::")
              path-elts
              :initial-value " // namespace ")
      (insert nsfini)
      (insert nsfini)
      (insert "#endif // " guard)
      (goto-char final))
    )
  )


(defun my-begin-source ()
  "Begin a C/C++ source file"
  (interactive)
  (let* ((source-prep (my-prepare-source))
         (path-elts (car source-prep))
         (copyright (cdr source-prep))
         (basename (car (my-split-current-filename)))
         )


    (bufstart)
    (if copyright
        (my-copyright copyright)
      (my-copyright))

    (let ((final nil) ;; final position
          (nsfini (if path-elts "\n" "")))

      ;; opening namespace stuff
      (insert nsfini)
      (if path-elts
          (progn
            (insert "#include \"")
            (mapc (lambda (n) (insert n "/"))
                  path-elts)
            (insert (downcase basename) ".hpp\"\n\n")))

      (mapc (lambda (n) (insert "namespace " n " { "))
            path-elts)

      (insert nsfini)

      (setq final (point))
      (newline)

      (bufend)
      ;; make sure the next stuff goes on its own line
      (if (not (equal (current-column) 0))
          (newline))

      ;; closing namespace stuff
      (mapc (lambda (n) (insert "}")) path-elts)
      (reduce (lambda (prefix n)
                (insert prefix n) "::")
              path-elts
              :initial-value " // namespace ")
      (insert nsfini)
      (goto-char final)
      )
    )
  )

(defcustom my-buffer-initialization-alist
      '(
        ("\\.[ih]\\(pp\\|xx\\)?$" . my-begin-header)
        ("\\.c\\(pp\\|xx\\)$" . my-begin-source)
        ("\\.\\(jam\\|\\html?\\|sh\\|py\\|rst\\|xml\\)$" . my-copyright)
        )
      "A list of pairs (PATTERN . FUNCTION) describing how to initialize an empty buffer whose
file name begins matches PATTERN."
      ':type 'alist
      )

(defcustom my-default-copyright
      'boost-copyright
      "A symbol naming a function which generates the default copyright message"
      ':type 'symbol
      )

(if on-emacs
(defadvice find-file (after my-gud-translate-cygwin-paths activate)
  ;; if the file doesn't exist yet and is empty
  (if (and (equal (buffer-size) 0)
           (not (file-exists-p (buffer-file-name))))

      ;; try to find an initialization function
      (let ((initializer
             (find-if
              (lambda (pair) (string-match (car pair) (buffer-file-name)))
              my-buffer-initialization-alist)))

        ;; if found, call it
        (if initializer
            (progn (eval (list (cdr initializer)))
                   (set-buffer-modified-p nil)))
      ))))

(defun my-at-preprocessor-directive-p ()
  "return non-nil if point is sitting at the beginning of a preprocessor directive name"
  (and
   (save-excursion
     (re-search-backward "^\\([ \t]*\\)#\\([ \t]*\\)" (line-beginning-position) t))
   (>= (point) (match-beginning 2))
   (<= (point) (match-end 2))
    ))

(defun my-preprocessor-indentation ()
  (save-excursion
    (beginning-of-line)
    (re-search-backward "^[ \t]*#[ \t]*" nil t)
    (goto-char (match-end 0))
    (+ (current-column)
       (if (looking-at "\\(if\\)\\|\\(el\\)") 1 0)))) ; fixme: was 1 0

(defun my-electric-pound-< ()
  (interactive)
  (my-maybe-insert-incude "<" ">"))

(defun my-electric-pound-quote ()
  (interactive)
  (my-maybe-insert-incude "\"" "\""))

(defun my-maybe-insert-incude (open close)
  (if (my-at-preprocessor-directive-p)
      (progn
        (move-to-column (my-preprocessor-indentation) t)
        (insert "include " open)
        (save-excursion
          (insert close)))
    (insert open)))

(defun my-electric-pound ()
  (interactive)
  (insert "#")
  (if (my-at-preprocessor-directive-p)
      (progn
        (delete-region (match-beginning 1) (match-end 1))
        (move-to-column (my-preprocessor-indentation) t))))

(defun my-electric-pound-e ()
  (interactive)

  (if (my-at-preprocessor-directive-p)
      (progn
        (move-to-column (max 1 (- (my-preprocessor-indentation) 1))))) ; #fixme: was 1
  (insert "e"))

(defun my-c-namespace-indent (langelem)
  "Used with c-set-offset, indents namespace scope elements 2 spaces
from the namespace declaration iff the open brace sits on a line by itself."
  (save-excursion
    (if (progn (goto-char (cdr langelem))
;               (setq column (current-column))
               (end-of-line)
               (while (and (search-backward "{" nil t)
                           (assoc 'incomment (c-guess-basic-syntax))))
               (skip-chars-backward " \t")
               (bolp))
        2)))

(defun my-c-backward-template-prelude ()
  "Back up over expressions that end with a template argument list.

Examples include:

        typename foo<bar>::baz::mumble

        foo(bar, baz).template bing
"
  (while
      (save-excursion
        ;; Inspect the previous token or balanced pair to
        ;; see whether to skip backwards over it
        (c-backward-syntactic-ws)
        (or
         ;; is it the end of a nested template argument list?
         (and
          (eq (char-before) ?>)
          (c-backward-token-2 1 t) ;; skips over balanced "<>" pairs
          (eq (char-after) ?<))

         (and
          (c-backward-token-2 1 t)
          (looking-at "[A-Za-z_\\[(.]\\|::\\|->"))))

    (c-backward-token-2 1 t)))

(defun my-lineup-first-template-args (langelem)
  "Align lines beginning with the first template argument.

To allow this function to be used in a list expression, nil is
returned if we don't appear to be in a template argument list.

Works with: template-args-cont."
  (let ((leading-comma (my-c-leading-comma-p)))
    (save-excursion
      (c-with-syntax-table c++-template-syntax-table
        (beginning-of-line)
        (backward-up-list 1)
        (if (eq (char-after) ?<)

            (progn
              (my-c-backward-template-prelude)

              (vector
               (+ (current-column)
                  (if leading-comma (/ c-basic-offset 2) c-basic-offset)
                  ))

              ))))))


(defun my-lineup-more-template-args (langelem)
  "Line up template argument lines under the first argument,
adjusting for leading commas. To allow this function to be used in
a list expression, nil is returned if there's no template
argument on the first line.

Works with: template-args-cont."
  (let ((result (c-lineup-template-args langelem)))
    (if (not (eq result nil))
        (if (my-c-leading-comma-p)
            (vector (- (aref result 0) (/ c-basic-offset 2)))
          result))))

(defun my-lineup-template-close (langelem)
  (save-excursion
    (c-with-syntax-table c++-template-syntax-table
      (beginning-of-line)
      (c-forward-syntactic-ws (c-point 'eol))
      (if (and
           (eq (char-after) ?>)
           (progn
             (forward-char)
             (c-backward-token-2 1 t)
             (eq (char-after) ?<)))
          (progn
            (my-c-backward-template-prelude)
            (vector (current-column)))))))

(defun my-c-electric-comma (arg)
  "Amend the regular comma insertion by possibly appending a
  space."
  (interactive "*P") ; Require a writable buffer/take a prefix arg in raw form

  ;; Do the regular action.  Perhaps we should be using defadvice here?
  (c-electric-semi&comma arg)

  ;; Insert the space if this comma is the first token on the line, or
  ;; if there are preceding commas followed by a space.
  (and (eq (char-before) ?,)
       (save-excursion
         (backward-char)
         (skip-syntax-backward " ")
         (bolp)
         )
       (insert " "))
  )

(defun my-c-electric-gt (arg)
  "Insert a greater-than character.
The line will be re-indented if the buffer is in C++ mode.
Exceptions are when a numeric argument is supplied, point is inside a
literal, or `c-syntactic-indentation' is nil, in which case the line
will not be re-indented."
  (interactive "*P")
  (let ((indentp (and c-syntactic-indentation
                      (not arg)
                      (not (c-in-literal))))
        ;; shut this up
        (c-echo-syntactic-information-p nil))
    (self-insert-command (prefix-numeric-value arg))
    (if indentp
        (indent-according-to-mode))))

(defun my-c-namespace-open-indent (langelem)
  "Used with c-set-offset, indents namespace opening braces to the
same indentation as the line on which the namespace declaration
starts."
  (save-excursion
    (goto-char (cdr langelem))
    (let ((column (current-column)))
      (beginning-of-line)
      (skip-chars-forward " \t")
      (- (current-column) column))))

(defun my-c-tab ()
  (interactive "*")
  (delete-region (car (my-selection)) (cdr (my-selection)))
  (c-indent-command)
  )


(defconst graehl-style
   '(
;        (c-offsets-alist . (
;                        (defun-block-intro . 2)
                         ;; ...no exceptions.
;                        (substatement-open . 0)
;                        (inline-open . 0)
;                        (comment-intro . 0)
                                        ;     ))
     (c-electric-pound . t)
     (c-syntactic-indentation-in-macros . t)
     (c-indent-comments-syntactically-p . t)
     (c-hanging-braces-alist . (
                                ;; We like hanging open braces.
                                (brace-list-open)
                                (brace-entry-open)
                                (statement-cont)
                                (substatement-open) ;after
                                (block-close) ;c-snug-do-if
                                (extern-lang-open) ;after
                                (inexpr-class-open) ;after
                                (inexpr-class-close) ;before
                                ))
     (c-echo-syntactic-information-p . t)
     (indent-tabs-mode . nil)
     )
   "graehl")

 (defun graehl-style-common-hook ()
;   (c-add-style "graehl" graehl-style t)
)


(defun my-c-mode-hook ()
  (c-set-style "bsd")
  (setq c-default-style "bsd"
;        c-backspace-function 'c-hungry-delete
        ;'backward-delete-char
        c-basic-offset 2
        c-tab-always-indent t)

  (modify-syntax-entry ?_ "w")

  ;; Add 2 spaces of indentation when the open brace is on a line by itself
  (c-set-offset 'innamespace 'my-c-namespace-indent)

  ;; indent solo opening braces to the same indentation as the line on
  ;; which the namespace starts
  (c-set-offset 'namespace-open 'my-c-namespace-open-indent)

  ;; indent access labels public/private/protected by 1 space, as in 'M'. I
  ;; kinda like that.
  (c-set-offset 'access-label -3)
  (local-set-key [(control tab)]     ; move to next tempo mark
                            'tempo-forward-mark)
;  (local-set-key (kbd "<delete>") 'c-hungry-delete-forward)

  ;;
  ;;fixup template indentation
  ;;
  (c-set-offset 'template-args-cont
                (quote
                 (my-lineup-more-template-args
                  my-lineup-template-close
                  my-lineup-first-template-args
                  +)))

  (set-variable 'c-backslash-max-column 200)

  (my-code-mode-hook)

;  (local-set-key [tab] 'my-c-tab)
;  (local-set-key [{] 'my-electric-braces)
  (local-set-key [?\M-{] "\C-q{")
  (local-set-key [(control ?{)] 'my-empty-braces)
;  (local-set-key [(meta \`)] 'my-cpp-toggle-src-hdr)
    (local-set-key [(meta \`)] 'sourcepair-load)
    (local-set-key [(control \`)] 'sourcepair-load)
  (local-set-key [?#] 'my-electric-pound)
  (local-set-key [?<] 'my-electric-pound-<)
  (local-set-key [?>] 'my-c-electric-gt)
  (local-set-key [?\"] 'my-electric-pound-quote)
  (local-set-key [?e] 'my-electric-pound-e)
  (local-set-key [?,] 'my-c-electric-comma)
  (local-set-key (kbd ";") 'self-insert-command)
  (make-local-variable 'parens-require-spaces)
  (setq parens-require-spaces nil)

  (c-toggle-auto-hungry-state 1)
   (define-key c-mode-base-map "\C-m" 'newline-and-indent)
   (define-key c-mode-base-map [(control j)] 'dabbrev-expand)
   (c-set-offset 'c 'c-lineup-C-comments)


   ;; Set the comments to start where they ought to.
   (setq-default c-comment-continuation-stars "* ")
;(graehl-style-common-hook)
)

(add-hook 'idl-mode-hook 'my-c-mode-hook)
(add-hook 'c-mode-hook 'my-c-mode-hook)
(add-hook 'cc-mode-hook 'my-c-mode-hook)
(add-hook 'c++-mode-hook 'my-c-mode-hook)
(add-hook 'java-mode-hook 'my-c-mode-hook)
(add-hook 'c-mode-common-hook 'my-c-mode-hook)

;; Since pretty much all my .h files are actually C++ headers, use c++-mode instead of
;; c-mode for these files.
(setq auto-mode-alist
      (cons '("\\.h$" . c++-mode) auto-mode-alist))


;;
;; makefile
;;
(defun my-makefile-mode-hook ()
  (font-lock-mode t)
  (show-paren-mode t)
  (setq indent-tabs-mode t)  ; Makefiles actually _need_ tabs :(
  (local-set-key [( control ?\( )] 'my-matching-paren)
  (local-set-key [return] 'newline-and-indent)
  (local-set-key [(control return)] 'newline)
)

(add-hook 'makefile-mode-hook 'my-makefile-mode-hook)

;; Cover .mak files and Dean's auto-generated .mk1 files
(setq auto-mode-alist
      (cons '("\\.mak$" . makefile-mode) auto-mode-alist))

(setq auto-mode-alist
      (cons '("\\.mk1$" . makefile-mode) auto-mode-alist))



;;
;; Path translation for cygwin
;;
(defun my-translate-cygwin-paths (file)
  "Adjust paths generated by cygwin so that they can be opened by tools running under emacs."

  ;; If it's not a windows system, or the file doesn't begin with /, don't do any filtering
  (if (and (eq system-type 'windows-nt) (string-match "^/" file))

      ;; Replace paths of the form /cygdrive/c/... or //c/... with c:/...
      (if (string-match "^\\(//\\|/cygdrive/\\)\\([a-zA-Z]\\)/" file)
          (setq file (file-truename (replace-match "\\2:/" t nil file)))

        ;; ELSE
        ;; Replace names of the form /... with <cygnus installation>/...
        ;; try to find the cygwin installation
        (let ((paths (parse-colon-path (getenv "path"))) ; Get $(PATH) from the environment
              (found nil))

          ;; While there are unprocessed paths and cygwin is not found
          (while (and (not found) paths)
            (let ((path (car paths))) ; grab the first path
              (setq paths (cdr paths)) ; walk down the list
              (if (and (string-match "/bin/?$" path) ; if it ends with /bin
                       (file-exists-p                ; and cygwin.bat is in the parent
                        (concat
                         (if (string-match "/$" path) path (concat path "/"))
                         "../cygwin.bat")))
                  (progn
                    (setq found t) ; done looping
                    (string-match "^\\(.*\\)/bin/?$" path)
                    (setq file (file-truename (concat (match-string 1 path) file))))
                ))))))
    file)

;; This "advice" is a way of hooking a function to supply additional
;; functionality. In this case, we want to pre-filter the argument to the
;; function gud-find-file which is used by the emacs debugging mode to open
;; files specified by debug info.
;(defadvice gud-find-file (before my-gud-translate-cygwin-paths activate)  (ad-set-arg 0 (my-translate-cygwin-paths (ad-get-arg 0)) t))
;(defadvice compilation-find-file (before my-compilation-translate-cygwin-paths activate)  (ad-set-arg 1 (my-translate-cygwin-paths (ad-get-arg 1)) t))
(require 'gud)
(defun my-gud-run-to-cursor ()
  (gud-tbreak)
  (gud-cont))


;;
;; Key bindings
;;

;; Navigation by words
;(global-set-key [(control ,)] 'backward-word)
;(global-set-key [(control \.)] 'forward-word)
(global-set-key (kbd "<C-,>") 'backward-word)
(global-set-key (kbd "<C-.>") 'forward-word)


;; Navigation to other windows (panes)
;(global-set-key "\C-x\C-n" 'other-window)  ; Normally bound to set-goal-column
(global-set-key "\C-x\C-p" 'my-other-window-backward) ; Normally bound to mark-page
(global-set-key "\C-x5a" 'my-add-todo-entry)

;; growing and shrinking windows (panes)
;;
;; These default bindings happen to be duplicated anyway
;; (e.g. meta left = control left = backward-word) so We're not losing anything
(global-set-key [(meta left)] 'shrink-window-horizontally)
(global-set-key [(meta right)] 'enlarge-window-horizontally)
(global-set-key [(meta up)] 'shrink-window)
(global-set-key [(meta down)] 'enlarge-window)

;; Miscellaneous
(global-set-key "\C-x\C-g" 'goto-line)
(global-set-key "\C-x\C-k" 'my-kill-buffer)
; (global-set-key [f3] 'eval-last-sexp)
(global-set-key "\C-xr\C-k" 'my-kill-rectangle)
(global-set-key "\C-xr\C-y" 'my-yank-replace-rectangle)
(global-set-key "\C-xr\C-w" 'my-save-rectangle)
(global-set-key "\C-x\M-Q" 'my-force-writable)



;; Compilation
(global-set-key [(f10)] 'recompile) ; my-recompile, my-compile
(global-set-key [(control f10)] 'compile)
(global-set-key [(meta f10)] 'mode-compile) ; mode-compile
(global-set-key [f12] 'next-error)
(global-set-key [(shift f12)] 'previous-error)
(global-set-key [(control f12)] 'first-error)

; (global-set-key [f12] 'ps-print-buffer)

;; Debugging
(add-hook 'gud-mode-hook
          '(lambda ()
             (local-set-key [home]        ; move to beginning of line, after prompt
                            'comint-bol)
             (local-set-key [up]          ; cycle backward through command history
                            '(lambda () (interactive)
                               (if (comint-after-pmark-p)
                                   (comint-previous-input 1)
                                 (prev-line 1))))
             (local-set-key [down]        ; cycle forward through command history
                            '(lambda () (interactive)
                               (if (comint-after-pmark-p)
                                   (comint-next-input 1)
                                 (forward-line 1))))
         (local-set-key [f2] 'gud-cont)
         (local-set-key [f11] 'gud-step)
         (local-set-key [f10] 'gud-next)
         (local-set-key [(shift f11)] 'gud-finish)
         (local-set-key [(control f10)] 'my-gud-run-to-cursor)
         (local-set-key [f9] 'gud-break)
         (local-set-key [(shift f9)] 'gud-remove)
             ))


;; Version control
; (global-set-key "\C-xvd" 'my-tlm-diff-latest)

;; This is the way I like it, but Windows (and M) users may prefer the
;; commented-out versions below.
(global-set-key [home] 'beginning-of-buffer)
(global-set-key [end] 'end-of-buffer)

;;(global-set-key [home] 'beginning-of-line)
;;(global-set-key [end] 'end-of-line)
;;(global-set-key [\C-home] 'beginning-of-buffer) ;; You can always use M-<
;;(global-set-key [\C-end] 'end-of-buffer) ;; You can always use M->

(global-set-key [( control ?\( )] 'my-matching-paren)

;; This is normally set to bring up a buffer list, but there are many other
;; ways to do this seldom-desired function (e.g. C-mouse1, or look at the
;; "Buffers" menu at the top of the frame).
(global-set-key "\C-x\C-b" 'my-switch-to-previous-buffer)


;; Lots of modes use the tab key to perform indentation. Sometimes you just want
;; to move to the right a bit when you've already got the line indented
(global-set-key [(control tab)] 'tab-to-tab-stop)

;; Dealing with my incorrigible Windows instincts
(global-set-key "\C-z" 'undo)   ; Normally this minimizes the emacs window;
                                        ; yikes!

;; I normally use the incantation "Alt-space N" to minimize MSWindows windows
;; from the keyboard, but that doesn't cooperate well with emacs, so I've
;; defined Meta-control-escape to do the same thing inside emacs.
(global-set-key [(meta control escape)] 'iconify-or-deiconify-frame)

;(global-set-key "\C-v" 'yank)   ; I'm always scrolling the window when I mean to paste
(global-unset-key [(mouse-2)])  ; I hit mouse-2 by mistake too often, pasting junk into my files
(global-set-key [(down-mouse-2)] 'mouse-drag-region)    ; Make it the same as mouse-1

;; Other useful strokes and commands
;; M-: (alt-shift-;) - evaluate lisp expression
;; C-x C-e - evaluate the preceding lisp expression on this line
;; edebug-<tab> a suite of elisp debugging functions (e.g. edebug-defun)
;; M-! (alt-shift-1) - do a shell command, e.g. tlm edit
;; C-x C-f (visit file) to make a buffer modifiable after you've 'tlm edited' it.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Changelog
;;
;; 8/16/01 - Cope with the default Metrowerks error messages, which are more
;;           descriptive than those you get with -msgstyle gcc. Also added the
;;           standard MSVC debugging keybindings.
;;
;; 8/02/01 - Added Matlab mode from Mark R., and fixed the setting of
;;           user-mail-address for altrabroadband users.
;;
;; 7/26/01 - Added a fix for vc-mode losing the writable status of checked-out
;;           files. Commented out obsolete TLM version control stuff.
;;
;; 7/21/01 - Added auto-fill-mode to my-sh-mode-hook
;;
;; 7/11/01 - Fixed cygwin path translation yet again
;;
;; 6/28/01 - Added cygwin path translation for compilation error messages.
;;
;; 6/13/01 - Added Jam debugging keys
;;
;; 5/31/01 - Updated ediff defaults
;;
;; 4/7/01 - Added sh-mode for Jam files.
;;
;; 3/6/01 - Added my-blockquote-preformatted, my-yank-code, and associated
;; helper functions which help with HTML editing of code fragments.
;;
;; 3/1/01 - Added my-translate-cygwin-paths which handles cygwin paths for GDB
;;
;; 2/1/01 - Enabled sgml-quick-keys
;;
;; 5/11/00 - Fixed my-recompile so that the initial invocation calls compile
;; interactively when there is no compile-command set.
;;
;; 5/12/00 - Added lazy-lock mode to eliminate fontification delays when a new
;; file is visited. Added installation instructions at top of file.
;;
;; 5/12/00 - Added and updated my-cpp-toggle-src-hdr and M-` binding (thanks to
;; Ken Steele).
;;
;; 5/19/00 - Added global-auto-revert-mode
;;
;; 6/8/00 - my-makefile-mode-hook was inactive. I activated it.
;;
;; 6/30/00 - Began automated TLM stuff with my-tlm-diff-latest
;;
;; 7/7/00 - Changed default indentation of C++ access-specifiers to -3
;;
;; 8/26/00 - Added rectangle manipulations
;;
;; 9/1/00 - Fixed return email address
;;
;; 9/18/00 - Forced lowercase results from directory-files for WinNT systems.
;;
;; 11/02/00 - Added smtpmail-smtp-server setting
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


(if on-emacs
      (let ((lines (car-safe (assq 'tool-bar-lines default-frame-alist))))

        ;; Workaround for emacs-2.3 alpha bug, additionally setting a string
        ;; value of 0 with key HKLM\Software\Gnu\Emacs\Emacs.Toolbar in the
        ;; registry.
        (when (and lines
                   (integerp lines)
                   (> lines 0))
          (add-hook 'window-setup-hook #'(lambda () (tool-bar-mode -1))))

        (setq-default tool-bar-mode nil)
        (setq-default default-frame-alist (quote ((tool-bar-lines . 0) (menu-bar-lines . 1))))
; '(default-frame-alist (quote ((tool-bar-lines . 0) (menu-bar-lines . 1))) t)

        )

  ;; else
  (progn
    (customize-set-variable 'paren-mode 'sexp)
    (customize-set-variable 'toolbar-visible-p nil)
  ))


(global-set-key [(meta \[)] 'split-window-horizontally)


; (and on-win32 (progn
;                         (setenv "GNUSERV_SHOW_EMACS" "1")
;                         (require 'gnuserv)
;                         (gnuserv-start)
;                         (setq gnuserv-frame (selected-frame))
;                         ))
(if (and nil emacs-22)
    (progn (require 'ido) (ido-mode t) (global-set-key [(f6)] 'ido-switch-buffer))
  (progn (require 'iswitchb) (global-set-key [(f6)] 'iswitchb-buffer)) (or recent-xemacs (iswitchb-default-keybindings))
  )

(global-set-key [f3] 'delete-char)
(global-set-key [(control h)] 'backward-kill-word)
(global-set-key [f4] 'iswitchb-buffer)
(global-set-key (kbd "ESC <f3>") 'kill-buffer)


(setq no-tramp t)
(or no-tramp recent-xemacs (progn
     (push "~/elisp/tramp/lisp" load-path)
     (push "~/elisp/tramp/contrib" load-path)
;(require 'tramp)
;(setq tramp-chunksize 500)
;(setq tramp-debug-buffer t)
;(setq tramp-verbose 10)
;(and on-win32 (setq tramp-auto-save-directory win32-tempdir))


(and nil
     (and on-win32 on-emacs (progn
                                (setq tramp-methods
                                      (cons '("sshj"
                                              (tramp-login-program "ssh")
                                              (tramp-copy-program nil)
                                              (tramp-remote-sh "/bin/bash -i")
                                              (tramp-login-args
                                                                                                    (("%h")
                                                                                                     ("-l" "%u")
                                                                                                     ("-p" "%p")
                                                                                                     ( "-i" "/cache/.ssh/identity" "-e" "none")))
                                              (tramp-copy-args nil)
                                              (tramp-copy-keep-date-arg nil)
                                              (tramp-password-end-of-line nil)
                                              )
                                            tramp-methods))
                                (setq tramp-default-method "sshj")
                                )
     )
     )

(and nil (and on-win32 on-emacs) (progn
                                (setq tramp-methods
                                      (cons '("sshj"
                                              (tramp-connection-function  tramp-open-connection-rsh)
;             (tramp-rsh-program          "c:\\cygwin\\bin\\ssh.exe")
;             (tramp-rcp-program          "c:\\cygwin\\bin\\scp.exe")
                                              (tramp-rsh-program          "ssh")
                                              (tramp-rcp-program          "scp")
                                              (tramp-remote-sh            "/bin/sh")
                                              (tramp-rsh-args     ("-e" "none" "-t" "-t" "-v" "-i" "/cache/.ssh/identity"))
                                              (tramp-rcp-args     ("-i" "/cache/.ssh/identity"))
                                              (tramp-rcp-keep-date-arg    "-p")
                                              ) tramp-methods))
                                (setq tramp-default-method "sshj")
                                )
     )

(and (not on-emacs) (setq process-connection-type t))
;(add-to-list 'Info-directory-list "~/elisp/tramp/info")
;(setq tramp-encoding-shell "bash")
;(setq tramp-encoding-shell "c:/cygwin/bin/bash.exe")
;/sshx:hpc-master.usc.edu/bin
;/sshj:hpc-master.usc.edu/bin
; (add-to-list      'tramp-multi-connection-function-alist      '("sshf" tramp-multi-connect-rlogin "ssh %h -l %u -i /cache/.ssh/identity %n"))
;/[hpc-master.usc.edu]/.emacs
;/[sshx/hpc-master.usc.edu]/.emacs

(defun tramp-compile (cmd)
  "Compile on remote host."
  (interactive "sCompile command: ")
  (save-excursion
    (pop-to-buffer (get-buffer-create "*Compilation*") t)
    (erase-buffer))
  (shell-command cmd (get-buffer "*Compilation*"))
  (pop-to-buffer (get-buffer "*Compilation*"))
  (compilation-minor-mode 1))

     ))
;(require 'ange-ftp)


(if (and on-emacs on-win32) (progn
     (setq exec-path (cons "C:/cygwin/bin" exec-path))
        (setenv "PATH" (concat "C:\\cygwin\\bin;" (getenv "PATH")))
     (setq exec-path (cons "z:/isd/cygwin/bin" exec-path))
        (setenv "PATH" (concat "z:\\isd\\cygwin\\bin;" (getenv "PATH")))
    ))



;; move cursor to the previous line or get previous history item,depending
;; on whether we're at a shell mode prompt
(defun ewd-comint-up (arg)
  (interactive "p")
  (if (comint-after-pmark-p)
      (comint-previous-input arg)
    (prev-line arg)))

;; move cursor to the next line or get next history item, depending
;; on whether we're at a shell mode prompt
(defun ewd-comint-down (arg)
  (interactive "p")
  (if (comint-after-pmark-p)
      (comint-next-input arg)
    (forward-line arg)))

;; bind my special functions to the up and down keys in shell-mode
(add-hook 'shell-mode-hook
          (lambda ()
;            (define-key shell-mode-map [up] 'ewd-comint-up)
;            (define-key shell-mode-map [down] 'ewd-comint-down)
            (define-key shell-mode-map [(control k)] 'comint-kill-input)
            (local-set-key [home]        ; move to beginning of line, after prompt
                            'comint-bol)
	     (local-set-key [up]          ; cycle backward through command history
                            '(lambda () (interactive)
                               (if (comint-after-pmark-p)
                                   (comint-previous-input 1)
                                 (prev-line 1))))
	     (local-set-key [down]        ; cycle forward through command history
                            '(lambda () (interactive)
                               (if (comint-after-pmark-p)
                                   (comint-next-input 1)
                                 (forward-line 1))))
            (and on-win32 (progn
                              (setq comint-scroll-show-maximum-output 'this)
                              (setq comint-completion-addsuffix t)
                              ;; (setq comint-process-echoes t) ;; reported that this is no longer needed
                              (setq comint-eol-on-send t)
;          (setq w32-quote-process-args ?\")
                              (make-variable-buffer-local 'comint-completion-addsuffix)))
            )
)


                                        ;(setq shell-command-switch "-l")
;(setq w32-quote-process-args ?\")
(setq shell-file-name "bash")
;(setq shell-file-name "cmdproxy.exe")
(if t
    ;(not cygwin-xemacs)
    (progn
(setenv "PID" nil)
(setenv "SHELL" shell-file-name)
(setenv "COMSPEC" shell-file-name)
)
)
(setq explicit-shell-file-name shell-file-name)

;; For subprocesses invoked via the shell
;; (e.g., "shell -c command")

;; The emacs command 'shell' normally brings you back to the same
;; *Shell* buffer every time.
(defun new-shell()
  "Start up a new shell in a uniquely-named buffer."
  (interactive)
  (shell)
  ;;
  ;; Look for the process that exists for the now current buffer. Rename
  ;; it to include its process ID.
  ;;
  (let ((procs (process-list))
        (aProc nil)
        (buf (current-buffer))
        (pid nil))
    (while procs
      (setq aProc (car procs)
            procs (cdr procs))
      (if (eq buf (process-buffer aProc))
          (setq pid (process-id aProc)
                procs nil)))
    (if pid
        (rename-buffer (format "%s (%d)" shell-file-name pid))
      (rename-buffer "shell" t))))

(global-set-key [(control \?)] 'new-shell)

(set-face-foreground 'font-lock-type-face "steelblue")
(set-face-foreground 'font-lock-keyword-face "red")
(set-face-foreground 'font-lock-function-name-face "red")
(set-face-foreground 'font-lock-variable-name-face "magenta4")
(set-face-foreground font-lock-builtin-face "red3")
(set-face-foreground font-lock-comment-face "red3")
(set-face-foreground font-lock-constant-face "slateblue")
(set-face-foreground font-lock-string-face "darkgreen")
;(set-face-foreground 'font-lock-string-face "green4")
(define-key cperl-mode-map "\C-cy" 'cperl-check-syntax)

(setq my-kbd-macro 1)
(if my-kbd-macro
    (progn
(global-set-key  [(meta left)] 'backward-sexp)
(global-set-key  [(meta right)] 'forward-sexp)
(global-set-key  [(control tab)] 'bs-cycle-next)
(global-set-key  [(control shift tab)] 'bs-cycle-previous)
(global-set-key  [(meta return)] 'dabbrev-expand)
(global-set-key  [(control shift tab)] 'bs-cycle-previous)
(global-set-key  [(control shift prior)] 'upcase-word)
(global-set-key  [(control shift next)] 'downcase-word)
)
)

 (defun slash-to-backslash (text)
   (substitute ?\\ ?/ text))

;(defun fume-add-menubar-entry t)
;(defun fume-add-menubar-entry (&optional force)  (interactive) t)

(defun revert-all-buffers()
      "Refreshs all open buffers from their respective files"
      (interactive)
      (let* ((list (buffer-list))
	      (buffer (car list)))
        (while buffer
          (if (string-match "\\*" (buffer-name buffer))
	      (progn
	        (setq list (cdr list))
	        (setq buffer (car list)))
	      (progn
	        (set-buffer buffer)
	        (revert-buffer t t t)
	        (setq list (cdr list))
	        (setq buffer (car list))))))
      (message "Refreshing open files"))

;(require `browse-kill-ring)

;; Mark ring size
(setq mark-ring-max 32)			; default is 16

;; Kill ring size
(setq kill-ring-max 200)			; default is 30

;; Search ring size
(setq search-ring-max 32)		; default is 16

(auto-compression-mode 1)


;; awesome: adds arrow keys to iswitchb
(require 'edmacro)
    (defun iswitchb-local-keys ()
      (mapc (lambda (K)
	      (let* ((key (car K)) (fun (cdr K)))
    	        (define-key iswitchb-mode-map (edmacro-parse-keys key) fun)))
	    '(("<right>" . iswitchb-next-match)
	      ("<left>"  . iswitchb-prev-match)
	      ("<up>"    . ignore             )
	      ("<down>"  . ignore             ))))
    (add-hook 'iswitchb-define-mode-map-hook 'iswitchb-local-keys)
;; /awesome

;(global-set-key [(control c) (control r)] 'query-replace-regexp)
;(global-set-key [(control f1)] 'cvs-update)
;(global-set-key [(meta f1)] 'cvs-status)
(global-set-key [(control f1)] 'query-replace-regexp)

;;; (setq shell-load-hook        '((lambda ()        (require 'comint-isearch)        (define-key shell-mode-map "\C-r" 'comint-isearch))))
(define-key shell-mode-map "\C-r" 'isearch-backward)

(setq sourcepair-source-path    '( "impl" "." "~/t/sbmt_decoder/src" "../src"))
(setq sourcepair-header-path    '( "." "include" "../include" )) ;;"~/t/sbmt_decoder/include/sbmt" "~/t/sbmt_decoder/include/sbmt/ngram"
(setq sourcepair-recurse-ignore '( "CVS" "Obj" "Debug" "Release" ".svn"))
;;    (global-set-key [(meta \`)] 'sourcepair-load)




;; scratch.el


(defvar pre-scratch-buffer nil
"Name of active buffer before switching to the scratch buffer.")

(defvar pre-scratch-config nil
"Window configuration before switching to the scratch buffer.")

(defun scratch (&optional buffername initmode &rest minormodes)
  "Access an existing scratch buffer or create one anew if none exists.
This is similar to the function `shell' for accessing or creating
an interactive shell.

Optional argument BUFFERNAME specifies the scratch buffer name;
the default buffer name is *scratch*.

Optional argument INITMODE specifies the major mode in which
scratch buffer should be initialized; the default mode is
`initial-major-mode'.

Any remaining arguments are MINORMODES to apply in the scratch
buffer."
  (interactive)
  (pre-scratch-setup)
  (let* ((name (if buffername
                   buffername
                 "*scratch*"))
         (live (buffer-live-p (get-buffer name)))
         (buffer (get-buffer-create name)))
    (pop-to-buffer buffer)
    ;; switch to `lisp-interaction-mode' (or whatever the initial
    ;; major mode is) if the buffer is in `fundamental-mode' (or
    ;; whatever the default major mode is)...
    (if (equal mode-name (with-temp-buffer (funcall default-major-mode)
                                           mode-name))
        ;; but only if the buffer has not been fiddled with
        (unless live
          (funcall (if initmode
                       initmode
                     initial-major-mode))
          ;; also take the opportunity to set up any minor modes
          ;; that were requested
          (mapc 'funcall minormodes)))
    buffer))

(defun pre-scratch-setup ()
  (setq pre-scratch-buffer (buffer-name (current-buffer)))
  (setq pre-scratch-config (current-window-configuration)))

(defun back-from-scratch ()
  "Used after running `scratch' to restore the window
configuration to the state it was in beforehand."
  (interactive)
  (if (not (get-buffer pre-scratch-buffer))
      (message "Buffer %s no longer exists!" pre-scratch-buffer)
    (set-window-configuration pre-scratch-config)))

;;; Example Configuration:

;; (Comment this out if you don't like it!)

(defun messages () (interactive)
  (scratch "*Messages*" 'fundamental-mode)
  (goto-char (point-max))
  (recenter))

;; better than `shell' because you can jump back quickly.
(defun shel () (interactive)
  (if (buffer-live-p (get-buffer "*shell*"))
      (scratch "*shell*")
    (pre-scratch-setup)
    (shell)))

(global-set-key [(meta f2)] 'scratch)
(global-set-key [(meta f3)] 'back-from-scratch)

;(global-set-key "\C-w" 'backward-kill-word)
(global-set-key "\C-w" 'kill-region)
(global-set-key "\C-x\C-k" 'kill-region)

;(if (fboundp 'scroll-bar-mode) (scroll-bar-mode -1))
(if (fboundp 'tool-bar-mode) (tool-bar-mode -1))
;(if (fboundp 'menu-bar-mode) (menu-bar-mode -1))
(and on-emacs (set-scroll-bar-mode nil))

; c-x c-n ... c-u c-x c-n
(put 'set-goal-column 'disabled nil)
(global-set-key "\C-x\C-n" 'set-goal-column)

; haskell
(global-set-key (kbd "C-x a r") 'align-regexp)
;(add-to-list 'auto-mode-alist '("\\.hs\\'" . haskell-mode))
(setq auto-mode-alist
      (append auto-mode-alist
              '(("\\.[hg]s$"  . haskell-mode)
                ("\\.hi$"     . haskell-mode)
                ("\\.l[hg]s$" . literate-haskell-mode))))

(autoload 'haskell-mode "haskell-mode"
   "Major mode for editing Haskell scripts." t)
(autoload 'literate-haskell-mode "haskell-mode"
   "Major mode for editing literate Haskell scripts." t)
(setq-default indent-tabs-mode nil)

;(remove-hook 'haskell-mode-hook 'turn-on-haskell-indent)
;; Just use tab-stop indentation, 2-space tabs

(defun newline-and-indent-relative ()
(interactive)
(newline)
(indent-to-column (save-excursion
(prev-line 1)
(back-to-indentation)
(current-column))))

;(load "~/elisp/haskell-ghci.el")
;(add-hook 'haskell-mode-hook 'turn-on-haskell-ghci)

(setq temporary-file-directory "/tmp")
(and on-win32 (setq temporary-file-directory "c:/cygwin/tmp"))
(load "~/elisp/haskell-mode-2.4/haskell-site-file.el")
(setq haskell-ghci-program-name "c:/ghc/ghc-6.10.2/bin/ghci.exe")
;(setq haskell-font-lock-symbols nil)
(require 'inf-haskell)
(setq require-final-newline t)
(fset 'yes-or-no-p 'y-or-n-p)
(setq next-line-add-newlines nil)
(setq-default indent-tabs-mode nil)
(setq query-replace-highlight t)
; Moving cursor down at bottom scrolls only a single line, not half page
(setq scroll-step 10)
(setq scroll-conservatively 5)
(global-set-key [delete] 'delete-char)
(setq kill-whole-line t)
;(setq c-hungry-delete-key t)
(setq c-auto-newline 1)

; capitalize current word (for example, C constants)
(global-set-key "\M-u"        '(lambda () (interactive) (backward-word 1) (upcase-word 1)))
(global-set-key "\M-l"        '(lambda () (interactive) (backward-word 1) (downcase-word 1)))
(global-set-key "\C-m"        'newline-and-indent)
(global-set-key "\M-\C-u" 'turn-on-auto-capitalize-mode)

; auto-capitalize stuff
(autoload 'auto-capitalize-mode "auto-capitalize"
  "Toggle `auto-capitalize' minor mode in this buffer." t)
(autoload 'turn-on-auto-capitalize-mode "auto-capitalize"
  "Turn on `auto-capitalize' minor mode in this buffer." t)
(autoload 'enable-auto-capitalize-mode "auto-capitalize"
  "Enable `auto-capitalize' minor mode in this buffer." t)
;(add-hook 'text-mode-hook 'turn-on-auto-capitalize-mode)
(setq auto-capitalize-words '("I" "Jonathan" "Graehl"))
;; Abbreviations

;; M-x edit-abbrevs        allows editing of abbrevs
;; M-x write-abbrev-file   will save abbrevs to file
;; C-x a i l               allows us to define a local abbrev
;; M-x abbrev-mode         turns abbrev-mode on/off

;; set name of abbrev file with .el extension
(setq abbrev-file-name "~/.abbrevs.el")

;(setq-default abbrev-mode t)
(setq save-abbrevs t)
;; we want abbrev mode in all modes (does not seem to work)
;; (abbrev-mode 1)
;; quietly read the abbrev file
;; (quietly-read-abbrev-file)
(if (file-exists-p  abbrev-file-name) (quietly-read-abbrev-file abbrev-file-name))

;(global-set-key "\C-h" 'backward-delete-char)
(add-hook 'emacs-lisp-mode-hook
          '(lambda ()
             (modify-syntax-entry ?- "w")       ; now '-' is not considered a word-delimiter
             ))

(global-set-key [(meta control backspace)] 'delete-rectangle)
(global-set-key [(meta shift h)] 'mark-sexp)
(require 'thingatpt+)
(defun rgr/hayoo()

  (interactive)
 (let* ((default (region-or-word-at-point))
(term (read-string (format "Hayoo for the following phrase (%s): "

                                   default))))
   (let ((term (if (zerop(length term)) default term)))
     (browse-url (format "http://holumbus.fh-wedel.de/hayoo/hayoo.html?query=%s&submit=Search" term)))))
;(require 'hs-lint)
(define-key haskell-mode-map (kbd "<f3>") (lambda()(interactive)(rgr/hayoo)))
(define-key haskell-mode-map (kbd "<f2>") 'haskell-hoogle)
;(define-key haskell-mode-map (kbd "<f4>") 'hs-lint)
;(add-hook 'find-file-hooks 'fume-setup-buffer)
;(add-hook 'Manual-mode-hook 'turn-on-fume-mode)

(add-hook 'haskell-mode-hook
(lambda ()
;  (my-code-mode-hook)
(turn-on-haskell-decl-scan)
(turn-on-haskell-doc-mode)
;(turn-on-haskell-simple-indent)
(turn-on-haskell-indent)
(turn-on-font-lock)
(setq haskell-indent-offset 2)
(setq inferior-haskell-wait-and-jump t)

;(setq indent-line-function 'tab-to-tab-stop)
(setq tab-stop-list
(loop for i from 2 upto 120 by 2 collect i))
;(local-set-key (kbd "RET") 'newline-and-indent-relative)
;(local-set-key [(tab)] 'haskell-indent-cycle)
;(local-set-key [(control Y)] 'haskell-ghci-load-file)
(local-set-key [(control Y)] 'inferior-haskell-load-file)
))
(require 'goto-addr)
(add-hook 'haskell-mode-hook 'imenu-add-menubar-index)
(when on-xemacs (add-hook 'haskell-mode-hook 'haskell-ds-func-menu))
(require 'haskell-decl-scan)
(define-key haskell-mode-map (kbd "<C-up>") 'haskell-ds-backward-decl)
(define-key haskell-mode-map (kbd "<C-down>") 'haskell-ds-forward-decl)
(define-key haskell-mode-map  [(shift backspace)] 'delete-indentation)
(define-key haskell-mode-map  [(tab)] 'haskell-indent-cycle)
(define-key haskell-mode-map (kbd "C-c t") 'inferior-haskell-type)
(require 'rect)


(when on-win32 (require 'cygwin-mount)  (setq cygwin-mount-cygwin-bin-directory "C:/cygwin/bin")  (cygwin-mount-activate))
(column-number-mode t)
(defun my-height (h)
  (when on-win32
    (if (and nil on-emacs)
        (set-screen-height (+ 1 h))
      (set-frame-height nil h)
      ))
  )
(when on-win32-emacs (server-mode t))

(when on-win32-emacs
(require 'flymake)

(defun credmp/flymake-display-err-minibuf ()
  "Displays the error/warning for the current line in the minibuffer"
  (interactive)
  (let* ((line-no             (flymake-current-line-no))
         (line-err-info-list  (nth 0 (flymake-find-err-info flymake-err-info line-no)))
         (count               (length line-err-info-list))
         )
    (while (> count 0)
      (when line-err-info-list
        (let* ((file       (flymake-ler-file (nth (1- count) line-err-info-list)))
               (full-file  (flymake-ler-full-file (nth (1- count) line-err-info-list)))
               (text (flymake-ler-text (nth (1- count) line-err-info-list)))
               (line       (flymake-ler-line (nth (1- count) line-err-info-list))))
          (message "[%s] %s" line text)
          )
        )
      (setq count (1- count)))))

(add-hook
 'haskell-mode-hook
 '(lambda ()
    (define-key haskell-mode-map "\C-cd"
      'credmp/flymake-display-err-minibuf)))

;Alternative version using pure elisp AOP

    (defun flymake-Haskell-init ()
          (flymake-simple-make-init-impl
            'flymake-create-temp-with-folder-structure nil nil
            (file-name-nondirectory buffer-file-name)
            'flymake-get-Haskell-cmdline))

    (defun flymake-get-Haskell-cmdline (source base-dir)
      (list "ghc"
    	(list "--make" "-fbyte-code"
    	      (concat "-i"base-dir)  ;;; can be expanded for additional -i options as in the Perl script
    	      source)))

    (defvar multiline-flymake-mode nil)
    (defvar flymake-split-output-multiline nil)

    ;; this needs to be advised as flymake-split-string is used in other places and I don't know of a better way to get at the caller's details
    (defadvice flymake-split-output
      (around flymake-split-output-multiline activate protect)
      (if multiline-flymake-mode
          (let ((flymake-split-output-multiline t))
    	    ad-do-it)
        ad-do-it))

    (defadvice flymake-split-string
      (before flymake-split-string-multiline activate)
      (when flymake-split-output-multiline
        (ad-set-arg 1 "^\\s *$")))

    (add-hook
     'haskell-mode-hook
     '(lambda ()
     	;;; use add-to-list rather than push to avoid growing the list for every Haskell file loaded
        (add-to-list 'flymake-allowed-file-name-masks
    		 '("\\.l?hs$" flymake-Haskell-init flymake-simple-java-cleanup))
        (add-to-list 'flymake-err-line-patterns
    		 '("^\\(.+\\.l?hs\\):\\([0-9]+\\):\\([0-9]+\\):\\(\\(?:.\\|\\W\\)+\\)"
    		   1 2 3 4))
        (set (make-local-variable 'multiline-flymake-mode) t)))
)
;(my-height 60)
(delete-other-windows)
(split-window-horizontally)
(when nil
   ;; want two windows at startup
(other-window 1)              ;; move to other window
;(shell)                       ;; start a shell
;(rename-buffer "shell-first") ;; rename it
(other-window 1)              ;; move back to first window
)
(when nil
(require 'yasnippet)
(yas/initialize)
(yas/load-directory "~/elisp/snippets")
)
;(require 'tramp)

(global-set-key [(control |)] 'revert-buffer)
(define-key global-map [(control :)] 'comment-or-uncomment-region)
(require 'grep-buffers)
(require 'shebang)



(defun remove-dos-eol ()
  "Removes the disturbing '^M' showing up in files containing mixed UNIX and DOS line endings."
  (interactive)
  (setq buffer-display-table (make-display-table))
  (aset buffer-display-table ?\^M []))
(remove-dos-eol)
;(add-hook 'compilation-mode-hook 'remove-dos-eol)


;(setq compile-command "scalac -optimise breathalyzer.scala && java -server -batch -cp '.;c:/scala/lib/scala-library.jar' breathalyzer ../12.in ../short.txt")

;(setq compile-command "~/bin/scala-jar.sh breathalyzer ../2.in ../short.txt")
;(setq compile-command "bash --login -c 'cd c:/scala/projects/breathalyzer/src;scala-jar.sh breathalyzer.scala'")
;(setq compile-command "bash --login -c 'cd c:/scala/svn/test/files/run; scala-jar.sh t_lines.scala'")

(defun tweakemacs-delete-region-or-char ()
  "Delete a region or a single character."
  (interactive)
  (if mark-active
      (delete-region (region-beginning) (region-end))
    (delete-char 1)))
(global-set-key (kbd "C-d") 'tweakemacs-delete-region-or-char)


(defvar previous-column nil)
(defun nuke-line ()
  "Delete current line."
  (interactive)
  (setq previous-column (current-column))
  (delete-region (line-beginning-position) (line-end-position))
  (delete-char 1)
  (move-to-column previous-column))
(global-set-key (kbd "M-o") 'nuke-line)

(defun nuke-line ()
  "Delete current line."
  (interactive)
  (setq previous-column (current-column))
  (delete-region (line-beginning-position) (line-end-position))
  (delete-char 1)
  (move-to-column previous-column))

(defun tweakemacs-move-one-line-downward ()
  "Move current line downward once."
  (interactive)
  (forward-line)
  (transpose-lines 1)
  (prev-line 1))
(global-set-key [C-M-down] 'tweakemacs-move-one-line-downward)

(defun tweakemacs-move-one-line-upward ()
  "Move current line upward once."
  (interactive)
  (transpose-lines 1)
  (forward-line -2))
(global-set-key [C-M-up] 'tweakemacs-move-one-line-upward)

(defun tweakemacs-comment-dwim-region-or-one-line (arg)
  "When a region exists, execute comment-dwim, or if comment or uncomment the current line according to if the current line is a comment."
  (interactive "*P")
  (if mark-active
      (comment-dwim arg)
    (save-excursion
      (let ((has-comment? (progn (beginning-of-line) (looking-at (concat "\\s-*" (regexp-quote comment-start))))))
	(push-mark (point) nil t)
	(end-of-line)
	(if has-comment?
	    (uncomment-region (mark) (point))
	  (comment-region (mark) (point)))))))
(global-set-key (kbd "C-/") 'tweakemacs-comment-dwim-region-or-one-line)
(global-set-key (kbd "M-;") 'comment-dwim)
(global-set-key (kbd "M-,") 'transpose-sexps)
;m-z = zap-to-char


(global-set-key (kbd "C-x K") 'kill-others)
(defun kill-others () "" (interactive) (kill-other-buffers-of-this-file-name nil))
(defun kill-other-buffers-of-this-file-name (&optional buffername)
"Kill all other buffers visiting files of the same base name."
(interactive "bBuffer to make unique: ")
(let ((buffer (if buffername (get-buffer buffername) (current-buffer))))
(cond ((buffer-file-name buffer)
        (let ((name (file-name-nondirectory (buffer-file-name buffer))))
          (loop for ob in (buffer-list)
               do (if (and (not (eq ob buffer))
                           (buffer-file-name ob)
                           (let ((ob-file-name (file-name-nondirectory (buffer-file-name ob))))
                             (or (equal ob-file-name name)
                                 (string-match (concat name "\\(\\.~.*\\)?~$") ob-file-name))) )
                      (kill-buffer ob)))))
      (message "This buffer has no file name."))))



(if on-win32 (progn
               (require 'bookmarknav)
;(global-set-key [(mouse-4)] 'previous-buffer)
;(global-set-key [(mouse-5)] 'next-buffer)
(define-key global-map [(mouse-4)] 'bookmarknav-go-back)
(define-key global-map [(mouse-5)] 'bookmarknav-go-forward)
)
(progn
(global-set-key [(mouse-4)] 'mwheel-scroll)
(global-set-key [(mouse-5)] 'mwheel-scroll)
)
)

(require 'mode-compile)
(add-hook 'LaTeX-mode-hook 'mysite-latex-mode-hook)
  (defun mysite-latex-mode-hook ()
    (kill-local-variable 'compile-command)
   )
(when nil  (setq mode-compile-modes-alist
       (append '((latex-mode . (tex-compile kill-compilation)))
               mode-compile-modes-alist)))


;(require 'lazy-lock)
;(setq font-lock-support-mode 'lazy-lock-mode)

(setq case-fold-search t)
(setq read-buffer-completion-ignore-case t)
(setq read-file-name-completion-ignore-case t)
(setq completion-ignore-case t)

;c:/scala/svn/src/library/scala/Math.scala


;(add-hook 'compilation-mode-hook 'follow-mode)


 ;; Close the compilation window if there was no error at all.
(when nil  (setq compilation-exit-message-function
        (lambda (status code msg)
          ;; If M-x compile exists with a 0
          (when (and (eq status 'exit) (zerop code))
            ;; then bury the *compilation* buffer, so that C-x b doesn't go there
  	  (bury-buffer "*compilation*")
  	  ;; and return to whatever were looking at before
  	  (replace-buffer-in-windows "*compilation*"))
          ;; Always return the anticipated result of compilation-exit-message-function
  	(cons msg code)))
)

;(setq compile-command "SCALA_HOME=c:/scala rebuild= z:/bin/scala-jar.sh breathalyzer.scala z:/puzzle/scala/breathalyzer/hbug.in")
;(setq compile-command "JAVA=z:/isd/cygwin/bin/java SCALA_HOME=c:/scala z:/bin/scala-jar.sh bayestag")
(load-file "~/elisp/local")
(require 'saveplace)
(setq-default save-place t)

(defalias 'qrr 'query-replace-regexp)
(defalias 'lml 'list-matching-lines)
(defalias 'dml 'delete-matching-lines)
(defalias 'rof 'recentf-open-files)

(defun move-to-char (arg char)
  "Move to ARG'th occurrence of CHAR.
Case is ignored if `case-fold-search' is non-nil in the current
buffer. Goes backward if ARG is negative; error if CHAR not
found. Trivial modification of zap-to-char from GNU Emacs
22.2.1."
  (interactive "p\ncMove to char: ")
  (when (char-table-p translation-table-for-input)
    (setq char (or (aref translation-table-for-input char) char)))
  (search-forward (char-to-string char) nil nil arg)
  ;(goto-char (match-beginning 0))
  )

(global-set-key [?\M-\C-z] #'move-to-char)
(setq initial-scratch-message "")

(and emacs23 (progn
(require 'eproject-extras)

(define-project-type cpp (generic)
  (look-for "Makefile")
  : relevant-files ("\\.cpp" "\\.c" "\\.hpp" "\\.h"))

(define-project-type perl (generic)
  (or (look-for "Makefile.PL") (look-for "Build.PL"))
  :relevant-files ("\\.pm$" "\\.t$" "\\.pl$" "\\.PL$"))

(defun h-file-create ()
  "Create a new h file.  Insert a infdef/define/endif block"
  (interactive)
  (let ((bn (buffer-name (current-buffer))))
  (if (or (equal (substring bn -2 ) ".h")
          (equal (substring bn -4 ) ".hpp"))
      (if (equal "" (buffer-string))
          (let ((sub "GRAEHL_SHARED__"(upcase (file-name-sans-extension bn)))
            (insert "#ifndef "n"\n#define "n"\n\n#endif")))))))

(defun c-file-enter ()
  "Expands all member functions in the corresponding .h file"
  (interactive)
  (let ((c-file (buffer-name))
        (h-file (concat (substring (buffer-name (current-buffer)) 0 -3 ) "h")))
    (if (equal (substring (buffer-name (current-buffer)) -4 ) ".cpp")
        (if (file-exists-p h-file)
              (expand-member-functions h-file c-file)))))

(add-hook 'c-mode-hook
          (lambda ()
            (unless (file-exists-p "Makefile")
              (set (make-local-variable 'compile-command)
                   (let ((file (file-name-nondirectory buffer-file-name)))
                     (format "%s -c -o %s.o %s %s %s"
                             (or (getenv "CC") "g++")
                             (file-name-sans-extension file)
                             (or (getenv "CPPFLAGS") "-DDEBUG=9")
                             (or (getenv "CFLAGS") "-ansi -pedantic -Wall -g")
                             file))))))

(define-project-type lisp (generic)
  (eproject--scan-parents-for file
    (lambda (directory)
      (let ((dname (file-name-nondirectory directory)))
        (file-exists-p (format "%s/%s.asd" directory dname)))))
  :relevant-files ("\\.lisp$" "\\.asd$"))
))
(global-set-key (kbd "<kp-divide>") 'delete-other-windows)

(defun follow-cygwin-symlink ()
  "Follow Cygwin symlinks.
Handles old-style (text file) and new-style (.lnk file) symlinks.
\(Non-Cygwin-symlink .lnk files, such as desktop shortcuts, are still
loaded as such.)"
  (save-excursion
    (goto-char 0)
    (if (looking-at
         "L\x000\x000\x000\x001\x014\x002\x000\x000\x000\x000\x000\x0C0\x000\x000\x000\x000\x000\x000\x046\x00C")
        (progn
          (re-search-forward
           "\x000\\([-A-Za-z0-9_\\.\\\\\\$%@(){}~!#^'`][-A-Za-z0-9_\\.\\\\\\$%@(){}~!#^'`]+\\)")
          (find-alternate-file (match-string 1)))
      (if (looking-at "!<symlink>")
          (progn
            (re-search-forward "!<symlink>\\(.*\\)\0")
            (find-alternate-file (match-string 1))))
      )))
;(require 'cygwin-mount)
;(cygwin-mount-activate)
;(add-hook 'find-file-hooks 'follow-cygwin-symlink)
;(add-hook 'find-file-not-found-hooks 'follow-cygwin-symlink)

(and use-longlines
(setq longlines-wrap-follows-window-size t)
(add-hook 'text-mode-hook 'longlines-mode)
)

(global-set-key (kbd "M-s") 'fixup-whitespace)

;; F# specific configs
;; hooked ocaml tuareg mode. If you do ML with mono e. g.
(add-to-list 'load-path "~/elisp/tuareg-mode")
;(push "~/elisp/tuareg-mode" load-path)
;(setq auto-mode-alist (cons '("\\.ml\\w?" . tuareg-mode) auto-mode-alist))
;(autoload 'tuareg-mode "tuareg" "Major mode for editing Caml code" t)
;(autoload 'camldebug "camldebug" "Run the Caml debugger" t)

    (autoload 'tuareg-mode "tuareg" "Major mode for editing Caml code" t)
    (autoload 'camldebug "camldebug" "Run the Caml debugger" t)
    (autoload 'tuareg-imenu-set-imenu "tuareg-imenu"
      "Configuration of imenu for tuareg" t)
    (add-hook 'tuareg-mode-hook 'tuareg-imenu-set-imenu)
    (setq auto-mode-alist
        (append '(("\\.ml[ily]?$" . tuareg-mode)
              ("\\.topml$" . tuareg-mode))
                  auto-mode-alist))

;; now we use *.fs files for this mode
(setq auto-mode-alist (cons '("\\.fs\\w?" . tuareg-mode) auto-mode-alist))

(add-hook 'tuareg-mode-hook
       '(lambda ()
       (set (make-local-variable 'compile-command)
        (concat "fsc \""
            (file-name-nondirectory buffer-file-name)
            "\""))))

(defun tuareg-find-alternate-file ()
  "Switch Implementation/Interface."
  (interactive)
  (let ((name (buffer-file-name)))
    (if (string-match "\\`\\(.*\\)\\.fs\\(i\\)?\\'" name)
    (find-file (concat (tuareg-match-string 1 name)
               (if (match-beginning 2) ".fs" ".fsi"))))))
;(global-set-key (kbd "C-d") 'c-hungry-delete)
;(global-set-key (kbd "DEL") 'c-hungry-delete-forward)
;(global-set-key (kbd "<backspace>") 'backward-delete-char)
;(require 'git)
;(require 'git-blame)
(require 'magit)
;(require 'tuareg-imenu)
(define-key isearch-mode-map '[backspace] 'isearch-delete-char)
;(require 'ensime)
;(add-hook 'scala-mode-hook 'ensime-scala-mode-hook)
;(setq tramp-default-method "ssh")
;(nconc (cadr (assq 'tramp-login-args (assoc "ssh" tramp-methods))) '(("bash" "-i")))
;(setcdr (assq 'tramp-remote-sh (assoc "ssh" tramp-methods)) '("bash -i"))
(require 'hobo)
(global-set-key (kbd "C-x f") 'hobo-find-file)
(global-set-key (kbd "C-/") 'c-comment-region)
(require 'skeleton)
(defun c-comment-region ()
  (interactive)
  "insert /* */ around region, no matter what (TODO: toggle comment state, idempotent, ??)"
  (save-excursion
    (let ((start (car (my-selection)))
          (finish (cdr (my-selection))))
      (goto-char finish)
      (insert " */")
      (goto-char start)
      (insert "/* ")
    )))
(require 'wrap-region)
(wrap-region-global-mode t)

(setq compile-command "bash -c '. ~/.bashrc;cd /nfs/topaz/graehl/sbmt/trunk/graehl/carmel && ecage make bin/cage/carmel.debug'")
(setq compile-command "bash --login -c 'cmakews'")
(setq compile-command "bash --login -c 'ecage dmakews'")
(setq compile-command "bash --login -c 'ecage \"cd t/graehl/sblm;./pcfg.py\"'")
(setq compile-command "ssh hpc1506 'boost=1_35_0 target=sblm//install boostsbmt'")
(require 'google-c-style)
(add-hook 'c-mode-common-hook 'google-set-c-style)
(add-hook 'c-mode-common-hook 'google-make-newline-indent)

(defun match-paren (arg)
  "Go to the matching paren if on a paren; otherwise insert %."
  (interactive "p")
  (cond ((looking-at "\\s\(") (forward-list 1) (backward-char 1))
        ((looking-at "\\s\)") (forward-char 1) (backward-list 1))
        (t (self-insert-command (or arg 1))))
  )


;(require 'ido)
;(ido-mode t)
;(ido-mode nil)
;(setq ido-enable-flex-matching t)
;(setq ido-create-new-buffer 'always)
;(define-key ido-file-completion-map "\C-w" 'ido-delete-backward-updir)
;(setq completion-ignored-extensions      (cons "*.aux" completion-ignored-extensions))
;(global-set-key (kbd "C-x f") 'ido-find-file)
;(global-set-key (kbd "C-x C-f") 'ido-fallback-command)
;(setq ido-auto-merge-work-directories-length -1)
;(require 'anything)
(require 'hideshowvis)
(defvar hs-toggle-all-last-val nil)
(defun hs-toggle-all ()
  (interactive)
  (make-local-variable 'hs-toggle-all-last-val)
  ;; only modify buffer local copy
  (setq hs-toggle-all-last-val (not hs-toggle-all-last-val))
  (if hs-toggle-all-last-val
      (hs-hide-all)
      (hs-show-all)))

(defun my-hs-minor-mode-hook ()
  (local-set-key (kbd "C-+" ) 'hs-toggle-hiding)
  (local-set-key (kbd "C-M-+" ) 'hs-toggle-all))

(add-hook 'hs-minor-mode-hook 'my-hs-minor-mode-hook)
(require 'hideshow-org)
(global-set-key "\C-cv" 'hideshowvis-minor-mode)
(global-set-key "\C-ch" 'hs-org/minor-mode)

(setq file-name-shadow-tty-properties '(invisible t))
(file-name-shadow-mode 1)

(require 'align)
(defun indent-and-align (beginning end)
  (interactive "r")
  (save-excursion
    (indent-region beginning end nil)
    (align beginning end)
    ))
(defun hagus-perl-indent-setup ()
  (setq cperl-extra-newline-before-brace t)
  (setq cperl-indent-level 4)
  (setq cperl-brace-offset -2)
  (hs-minor-mode)
  (cperl-set-style "linux"))

(add-hook 'cperl-mode-hook 'hagus-perl-indent-setup)

(defun run-buffer ()
  ;(args)
  (interactive)
;  (interactive "sArguments to script: ")
  (let ((args "-h"))
  (save-buffer)
  (shell-command (concat (buffer-file-name (current-buffer)) " " args " &")
)))

(global-set-key (kbd "\C-cy") 'run-buffer)
(require 'shell)
(require 'comint)
(or (server-start) 1)

;(global-set-key "%" 'match-paren)
(global-set-key "%" 'self-insert-command)

(add-hook 'completion-setup-hook
          (lambda () (run-at-time 3 nil
                                        ;(lambda () (delete-windows-on "*Completions*"))
                                  (lambda () (my-kill-buffer "*Completions*"))
                                  )))

(custom-set-variables
 '(comint-scroll-to-bottom-on-input t)  ; always insert at the bottom
 '(comint-scroll-to-bottom-on-output t) ; always add output at the bottom
 '(comint-scroll-show-maximum-output t) ; scroll to show max possible output
 '(comint-completion-autolist t)        ; show completion list when ambiguous
 '(comint-input-ignoredups t)           ; no duplicates in command history
 '(comint-completion-addsuffix t)       ; insert space/slash after file completion
 )
(remove-hook 'kill-buffer-query-functions 'server-kill-buffer-query-function)
(global-set-key [f12] 'next-error)
(defun search-next-error ()
  (interactive)
  (pop-to-buffer "*compilation*")
  (search-forward "error: "))
(global-set-key [(shift f12)] 'search-next-error)
(defun next-error-nof (&optional arg reset)
  "don't jump to file"
  (interactive)
  (let ((next-error-function (lambda (x y) t)))
    (next-error arg reset)))


(setq large-file-warning-threshold 50000000)


;(define-key py-mode-map "C-;" 'my-python-send-region)
;(global-set-key "\C-m" 'newline-and-indent)
;(global-set-key (kbd "C-;") 'my-python-send-region)
;(require 'python)


(require 'whole-line-or-region)

(defun whole-line-or-region-comment-dwim (prefix)
  "Call `comment-dwim' on region or PREFIX whole lines."
  (interactive "*p")
  (whole-line-or-region-call-with-prefix 'comment-dwim prefix nil t))
(global-set-key (kbd "C-;") 'whole-line-or-region-comment-dwim)

;;
;;  What this version does is override the normal behavior of the
;;  prefix arg to `comment-dwim', and instead uses it to indicate how
;;  many lines the whole-line version will comment out -- no prefix
;;  value is passed to the original function in this case.  This is
;;  the version that I use, as it's just more intuitive for me.

;;
;; PYTHON
;;
(defvar use-python-mode 1)

(defun my-python-mode-hook ()
  ;;(auto-fill-mode nil)
  ;;  (filladapt-mode t)
  (font-lock-mode t)
  (show-paren-mode t)
  (make-local-variable 'adaptive-fill-regexp)
  (setq adaptive-fill-regexp "^[     ]*# ")
  (local-set-key [return] 'newline-and-indent)
  (local-set-key [(control return)] 'newline)
  (local-set-key [( control ?\( )] 'my-matching-paren)
  (make-local-variable 'parens-require-spaces)
  (setq parens-require-spaces nil)
  (setq fill-column 132)
  )

(defun my-python-send-region (beg end)
  (interactive "r")
  (if (eq beg end)
      (python-send-region (point-at-bol) (point-at-eol))
      (python-send-region beg end)))

(defun my-python-send-region2 (&optional beg end)
  (interactive)
  (let ((beg (cond (beg)
                   ((region-active-p)
                    (region-beginning))
                   (t (line-beginning-position))))
        (end (cond (end)
                   ((region-active-p)
                    (copy-marker (region-end)))
                   (t (line-end-position)))))
    (python-send-region beg end)))

(if use-python-mode
    (progn
      (push "~/elisp/python-mode" load-path)
      (require 'python-mode)
(defun py-hook ()
  (interactive)
  (local-set-key (kbd "<M-right>") 'py-shift-region-right)
  (local-set-key (kbd "<M-left>") 'py-shift-region-left)
;  (local-set-key (kbd "C-;") 'my-python-send-region2)
  (local-set-key "\C-m" 'newline-and-indent)
  (eldoc-mode 1)
)
(add-hook 'python-mode-hook 'py-hook)

      (add-hook 'python-mode-hook 'my-python-mode-hook)
                                        ;(add-hook 'python-mode-hook '(lambda () (require 'pyvirtualenv)))
      (setq-default py-indent-offset 4)

;;; Electric Pairs
      (add-hook 'python-mode-hook
                (lambda ()
                                        ;      (define-key py-mode-map "\"" 'electric-pair)
                                        ;      (define-key py-mode-map "\'" 'electric-pair)
                                        ;      (define-key py-mode-map "(" 'electric-pair)
                                        ;      (define-key py-mode-map "[" 'electric-pair)
                                        ;      (define-key py-mode-map "{" 'electric-pair)
                  (define-key py-mode-map [(shift f10)] 'py-pychecker-run)
                  ))
      )
  (progn
    (require 'python)
    ;;
    ;; python -- creates a subprocess running Python. Stolen from python-mode.el
    ;;
    (defvar python-shell-font-lock-keywords
      (append '(("[][(){}]" . font-lock-constant-face))
              '(("^python%\\|^>\\|^(pdb)" . font-lock-constant-face))
              python-font-lock-keywords
              ))
    (defun python ()
      (interactive)
      (switch-to-buffer-other-window
       (apply 'make-comint py-which-bufname py-which-shell nil py-which-args))
      (make-local-variable 'comint-prompt-regexp)
      (make-local-variable 'font-lock-defaults)
      (setq comint-prompt-regexp "^python% \\|^> \\|^(pdb) "
            font-lock-defaults '(python-shell-font-lock-keywords t))
      (add-hook 'comint-output-filter-functions 'py-comint-output-filter-function)
      (set-syntax-table py-mode-syntax-table)
      (use-local-map py-shell-map)
      (local-set-key "\C-a" 'comint-bol)
      (local-set-key "\C-c\C-a" 'beginning-of-line)
      (python-mode)
      (font-lock-mode))

    ))
(defun clear-shell ()
   (interactive)
   (let ((comint-buffer-maximum-size 0))
     (comint-truncate-buffer)))
(require 'shell)
 (require 'term)
 (defun term-switch-to-shell-mode ()
  (interactive)
  (if (equal major-mode 'term-mode)
      (progn
        (shell-mode)
        (set-process-filter  (get-buffer-process (current-buffer)) 'comint-output-filter )
        (local-set-key (kbd "C-j") 'term-switch-to-shell-mode)
        (compilation-shell-minor-mode 1)
        (comint-send-input)
      )
    (progn
        (compilation-shell-minor-mode -1)
        (font-lock-mode -1)
        (set-process-filter  (get-buffer-process (current-buffer)) 'term-emulate-terminal)
        (term-mode)
        (term-char-mode)
        (term-send-raw-string (kbd "C-l"))
        )))
(define-key term-raw-map (kbd "C-j") 'term-switch-to-shell-mode)

(add-hook 'compilation-finish-functions 'my-change-tmp-to-nfs)
(defun my-change-tmp-to-nfs (buffer &optional stat)
  "change tmp to nfs"
  (interactive "b")
  (save-excursion
    (set-buffer buffer)
    (goto-char (point-min))
    (let ((buffer-read-only nil))
      (while (re-search-forward "/tmp/trunk.graehl/trunk/" nil t)
        (replace-match "~/t/")))))
